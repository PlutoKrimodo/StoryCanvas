-- ============================================================
-- StoryCanvas 增量迁移：对话式多图生成（Storyboard Chat）
--
-- 为既有数据库补上：
--   1. conversations / conversation_messages —— 登录用户的会话与消息
--   2. generation_batches —— 一次分镜确认对应的 N 张图父记录
--   3. generation_tasks.batch_id / panel_index / panel_text —— 任务归属与分镜顺序
--
-- 全新数据库请直接使用 sql/init.sql（已包含以上结构）。
--
-- 用法：
--   PGPASSWORD=你的密码 psql -h localhost -U storycanvas -d storycanvas -f sql/migrate_conversation.sql
-- ============================================================

CREATE TABLE IF NOT EXISTS conversations (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(120),
    style VARCHAR(32),
    size VARCHAR(20),
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_conversations_user
    ON conversations(user_id, updated_at DESC);

CREATE TABLE IF NOT EXISTS conversation_messages (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    conversation_id UUID NOT NULL REFERENCES conversations(id) ON DELETE CASCADE,
    role VARCHAR(16) NOT NULL CHECK (role IN ('user', 'assistant')),
    message_type VARCHAR(24) NOT NULL CHECK (message_type IN ('text', 'storyboard', 'images')),
    content TEXT,
    payload JSONB NOT NULL DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_conversation_messages_conversation
    ON conversation_messages(conversation_id, created_at);

CREATE TABLE IF NOT EXISTS generation_batches (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    conversation_id UUID REFERENCES conversations(id) ON DELETE SET NULL,
    message_id UUID REFERENCES conversation_messages(id) ON DELETE SET NULL,
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) NOT NULL DEFAULT 'draft'
        CHECK (status IN ('draft', 'pending', 'processing', 'completed', 'partial', 'failed', 'cancelled')),
    style VARCHAR(32) NOT NULL,
    size VARCHAR(20),
    seed INTEGER,
    panel_count INTEGER NOT NULL,
    panels JSONB NOT NULL DEFAULT '[]',
    consistency_prompt TEXT,
    negative_prompt TEXT,
    parsed_data JSONB,
    byok_llm BOOLEAN NOT NULL DEFAULT false,
    byok_image BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_generation_batches_user
    ON generation_batches(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_generation_batches_status
    ON generation_batches(status);

ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS batch_id UUID REFERENCES generation_batches(id) ON DELETE SET NULL;
ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS panel_index INTEGER;
ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS panel_text TEXT;
CREATE INDEX IF NOT EXISTS idx_generation_tasks_batch
    ON generation_tasks(batch_id, panel_index);

COMMENT ON TABLE conversations IS '对话式创作会话；游客不落库';
COMMENT ON TABLE conversation_messages IS '会话消息；payload 保存分镜、批次 ID 与图片列表';
COMMENT ON TABLE generation_batches IS '一次分镜确认对应的 N 张图父记录；N 个任务共享一致性设定';
COMMENT ON COLUMN generation_batches.panels IS '用户确认后的分镜数组，按 index 升序';
COMMENT ON COLUMN generation_tasks.batch_id IS '所属批次；为空表示旧版单张任务';
COMMENT ON COLUMN generation_tasks.panel_index IS '批次内分镜序号，从 0 开始';
