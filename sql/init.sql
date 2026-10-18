-- ============================================================
-- StoryCanvas 数据库初始化脚本（v2.0）
--
-- 范围：用户管理子系统 + 智能图像生成子系统
-- 说明：
--   - 在线编辑子系统已放弃，page_elements 表与 canvas_data 字段不再创建
--   - v2.2：book_pages 增加 image_id（绑定生成图，用于轻量多页 PDF 导出）
--   - gen_random_uuid() 为 PostgreSQL 13+ 内置函数，无需 uuid-ossp 扩展
--   - 密码仅存 bcrypt / PBKDF2 单向哈希，禁止明文
-- ============================================================

-- ==================== 用户表 ====================
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    -- 单向哈希：bcrypt（$2a$/$2b$，约 60 字符）或 pbkdf2_sha256（约 100 字符）
    password_hash VARCHAR(255) NOT NULL,
    avatar VARCHAR(500),
    is_active BOOLEAN DEFAULT true,
    -- Guest-first：游客以「影子用户」存在（user_id 外键仍 NOT NULL，沿用全部归属校验），
    -- 注册/登录后通过「认领」把生成结果归属转移到真实用户（见 docs/TODO.md T-1 能力层）
    is_guest BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_username ON users(username);

COMMENT ON COLUMN users.password_hash IS '密码单向哈希（bcrypt 或 pbkdf2_sha256），禁止明文/可逆加密';

-- ==================== 用户自带 AI 凭据（BYOK）====================
-- 每个用户每项能力（LLM / 图像）至多一条；api_key 仅以 AES-256-GCM 密文存储。
-- 明文在加密前附加 userId + capability 前缀，防止密文在用户/能力之间被调包。
CREATE TABLE user_ai_credentials (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    capability VARCHAR(10) NOT NULL CHECK (capability IN ('llm', 'image')),
    provider VARCHAR(32) NOT NULL,
    api_key_enc TEXT NOT NULL,
    api_key_hint VARCHAR(40) NOT NULL,
    api_base VARCHAR(500),
    model VARCHAR(120),
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id, capability)
);
CREATE INDEX idx_user_ai_credentials_user ON user_ai_credentials(user_id);
COMMENT ON TABLE user_ai_credentials IS '用户自带 AI 凭据（BYOK）；api_key_enc 仅存 AES-256-GCM 密文';
COMMENT ON COLUMN user_ai_credentials.api_key_hint IS '脱敏提示（如 sk-****abcd），禁止存放完整 Key';

-- ==================== 绘本表 ====================
CREATE TABLE books (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(200) NOT NULL,
    description TEXT,
    cover_image VARCHAR(500),
    status VARCHAR(20) DEFAULT 'draft' CHECK (status IN ('draft', 'published', 'archived')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_books_user_id ON books(user_id);
CREATE INDEX idx_books_status ON books(status);

-- ==================== 页容器表 ====================
-- 降级为「页容器」：仅记录页码、标题与缩略图，用于承载生成的图片
-- 原 canvas_data（Fabric.js 画布状态）随在线编辑子系统一并移除
CREATE TABLE book_pages (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    book_id UUID NOT NULL REFERENCES books(id) ON DELETE CASCADE,
    page_number INTEGER NOT NULL,
    title VARCHAR(200),
    thumbnail VARCHAR(500),
    -- 绑定该页使用的生成图（「保存到绘本」时写入，用于 PDF 导出）
    -- 外键引用 generated_images（其建表在后），故在文件末尾用 ALTER 补齐
    image_id UUID,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(book_id, page_number)
);
CREATE INDEX idx_book_pages_book_id ON book_pages(book_id);
CREATE INDEX idx_book_pages_image_id ON book_pages(image_id);

-- ==================== 角色表（第二阶段）====================
CREATE TABLE characters (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    age INTEGER,
    appearance TEXT,
    style_info JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_characters_user_id ON characters(user_id);

-- ==================== 对话与会话（Storyboard Chat）====================
CREATE TABLE conversations (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(120),
    style VARCHAR(32),
    size VARCHAR(20),
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_conversations_user ON conversations(user_id, updated_at DESC);

CREATE TABLE conversation_messages (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    conversation_id UUID NOT NULL REFERENCES conversations(id) ON DELETE CASCADE,
    role VARCHAR(16) NOT NULL CHECK (role IN ('user', 'assistant')),
    message_type VARCHAR(24) NOT NULL CHECK (message_type IN ('text', 'storyboard', 'images')),
    content TEXT,
    payload JSONB NOT NULL DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_conversation_messages_conversation
    ON conversation_messages(conversation_id, created_at);

-- 一次「分镜确认 → N 张图」的父记录；任务仍是一任务一图
CREATE TABLE generation_batches (
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
    -- BYOK 快照：批次创建时固化，N 个任务继承
    byok_llm BOOLEAN NOT NULL DEFAULT false,
    byok_image BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_generation_batches_user ON generation_batches(user_id, created_at DESC);
CREATE INDEX idx_generation_batches_status ON generation_batches(status);

-- ==================== 生成任务表 ====================
CREATE TABLE generation_tasks (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    book_id UUID REFERENCES books(id) ON DELETE SET NULL,
    page_id UUID REFERENCES book_pages(id) ON DELETE SET NULL,
    prompt TEXT NOT NULL,
    original_text TEXT,
    parsed_data JSONB,
    status VARCHAR(20) DEFAULT 'pending'
        CHECK (status IN ('pending', 'processing', 'completed', 'failed', 'cancelled')),
    result_url VARCHAR(500),
    error_message TEXT,
    model_used VARCHAR(100),
    -- BYOK 任务快照：创建时记录是否使用用户自带 Key，处理阶段据此决定是否可回退
    byok_llm BOOLEAN NOT NULL DEFAULT false,
    byok_image BOOLEAN NOT NULL DEFAULT false,
    -- 身份快照：游客任务只计入游客池，登录认领后也不改写
    is_guest BOOLEAN NOT NULL DEFAULT false,
    -- 批次编排：batch_id 为空表示旧版单张任务；panel_index 从 0 开始
    batch_id UUID REFERENCES generation_batches(id) ON DELETE SET NULL,
    panel_index INTEGER,
    panel_text TEXT,
    parameters JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP WITH TIME ZONE
);
CREATE INDEX idx_generation_tasks_user_id ON generation_tasks(user_id);
CREATE INDEX idx_generation_tasks_book_id ON generation_tasks(book_id);
CREATE INDEX idx_generation_tasks_status ON generation_tasks(status);
CREATE INDEX idx_generation_tasks_batch ON generation_tasks(batch_id, panel_index);
-- 游客池 / 登录池每日计数按身份 + 日期过滤
CREATE INDEX idx_generation_tasks_created_guest
    ON generation_tasks (is_guest, created_at);

-- ==================== 生成图片表 ====================
CREATE TABLE generated_images (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    task_id UUID NOT NULL REFERENCES generation_tasks(id) ON DELETE CASCADE,
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    original_url VARCHAR(500),
    file_path VARCHAR(500) NOT NULL,
    file_size INTEGER,
    width INTEGER,
    height INTEGER,
    format VARCHAR(20),
    prompt_used TEXT,
    parameters JSONB DEFAULT '{}',
    is_favorite BOOLEAN DEFAULT false,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_generated_images_task_id ON generated_images(task_id);
CREATE INDEX idx_generated_images_user_id ON generated_images(user_id);

-- book_pages.image_id 外键（generated_images 建表在后，这里补齐）
ALTER TABLE book_pages
    ADD CONSTRAINT fk_book_pages_image
    FOREIGN KEY (image_id) REFERENCES generated_images(id) ON DELETE SET NULL;

COMMENT ON COLUMN book_pages.image_id IS '该页绑定的生成图（用于 PDF 导出），图片删除时置空';

-- ==================== updated_at 自动维护 ====================
CREATE OR REPLACE FUNCTION set_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_books_updated_at
    BEFORE UPDATE ON books
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_book_pages_updated_at
    BEFORE UPDATE ON book_pages
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_generation_tasks_updated_at
    BEFORE UPDATE ON generation_tasks
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();
