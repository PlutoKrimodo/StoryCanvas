-- ============================================================
-- StoryCanvas 增量迁移：generation_tasks.status 约束补全
--
-- 背景：早期版本的 generation_tasks 表在 status 的 CHECK 约束里没有 'cancelled'，
--       导致调用「取消生成」时必然报：
--         ERROR: new row for relation "generation_tasks" violates check constraint
--                "generation_tasks_status_check"
--       表现为前端提示「取消生成失败」。
--
-- 全新数据库请直接使用 sql/init.sql（已包含 cancelled），无需执行本脚本。
--
-- 用法：
--   PGPASSWORD=你的密码 psql -h localhost -U storycanvas -d storycanvas \
--       -f sql/migrate_task_status.sql
-- ============================================================

ALTER TABLE generation_tasks
    DROP CONSTRAINT IF EXISTS generation_tasks_status_check;

ALTER TABLE generation_tasks
    ADD CONSTRAINT generation_tasks_status_check
    CHECK (status IN ('pending', 'processing', 'completed', 'failed', 'cancelled'));
