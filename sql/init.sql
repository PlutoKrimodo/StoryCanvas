-- ============================================================
-- StoryCanvas 数据库初始化脚本（v2.0）
--
-- 范围：用户管理子系统 + 智能图像生成子系统
-- 说明：
--   - 在线编辑子系统已放弃，page_elements 表与 canvas_data 字段不再创建
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
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_username ON users(username);

COMMENT ON COLUMN users.password_hash IS '密码单向哈希（bcrypt 或 pbkdf2_sha256），禁止明文/可逆加密';

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
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(book_id, page_number)
);
CREATE INDEX idx_book_pages_book_id ON book_pages(book_id);

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
    parameters JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP WITH TIME ZONE
);
CREATE INDEX idx_generation_tasks_user_id ON generation_tasks(user_id);
CREATE INDEX idx_generation_tasks_book_id ON generation_tasks(book_id);
CREATE INDEX idx_generation_tasks_status ON generation_tasks(status);

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
