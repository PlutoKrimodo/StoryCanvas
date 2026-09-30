-- ============================================================
-- StoryCanvas 增量迁移：T-1 Guest-first（能力层）
--
-- 为既有数据库补上 users.is_guest 列（影子游客用户标记）。
-- 全新数据库请直接使用 sql/init.sql（已包含该列），无需执行本脚本。
--
-- 用法：
--   PGPASSWORD=你的密码 psql -h localhost -U storycanvas -d storycanvas -f sql/migrate_guest.sql
-- ============================================================

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS is_guest BOOLEAN NOT NULL DEFAULT false;
