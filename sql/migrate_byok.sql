-- ============================================================
-- StoryCanvas 增量迁移：BYOK（用户自带 LLM / 图像 API Key）
--
-- 为既有数据库补上：
--   1. user_ai_credentials —— 用户凭据密文表（每用户每能力至多一条）
--   2. generation_tasks.byok_llm / byok_image —— 任务级快照，
--      避免「创建时免额度、处理时 Key 被删除」导致静默回退系统 Key
--
-- 全新数据库请直接使用 sql/init.sql（已包含以上结构），无需执行本脚本。
--
-- 用法：
--   PGPASSWORD=你的密码 psql -h localhost -U storycanvas -d storycanvas -f sql/migrate_byok.sql
-- ============================================================

CREATE TABLE IF NOT EXISTS user_ai_credentials (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    capability VARCHAR(10) NOT NULL CHECK (capability IN ('llm', 'image')),
    provider VARCHAR(32) NOT NULL,
    -- AES-256-GCM 密文（Base64(IV|Cipher|Tag)），明文含 userId + capability 绑定前缀
    api_key_enc TEXT NOT NULL,
    -- 脱敏展示用（sk-****abcd），便于用户识别是哪一把 Key
    api_key_hint VARCHAR(40) NOT NULL,
    api_base VARCHAR(500),
    model VARCHAR(120),
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id, capability)
);
CREATE INDEX IF NOT EXISTS idx_user_ai_credentials_user
    ON user_ai_credentials(user_id);

ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS byok_llm BOOLEAN NOT NULL DEFAULT false;
ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS byok_image BOOLEAN NOT NULL DEFAULT false;

COMMENT ON TABLE user_ai_credentials IS
    '用户自带 AI 凭据（BYOK）；api_key_enc 仅以 AES-256-GCM 密文存储，禁止明文';
COMMENT ON COLUMN user_ai_credentials.api_key_hint IS
    '脱敏提示，如 sk-****abcd；不得存放完整 Key';
COMMENT ON COLUMN generation_tasks.byok_llm IS
    '创建任务时是否使用用户自带 LLM Key（快照，处理阶段据此决定是否可回退）';
COMMENT ON COLUMN generation_tasks.byok_image IS
    '创建任务时是否使用用户自带图像 Key（快照）';
