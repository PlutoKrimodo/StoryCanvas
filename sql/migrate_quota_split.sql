-- ============================================================
-- 额度拆分迁移：generation_tasks 增加创建时身份快照
--
-- 目的：
--   - 游客任务只计入游客池（每日 20 张，所有游客共享）
--   - 登录用户任务只计入登录池（每日 50 张，所有登录用户共享）
--   - 双项 BYOK 任务不进入任何每日计数
--
-- 部署顺序：先执行本迁移，再重启新后端；否则新代码查询 is_guest 会失败。
-- 历史回填说明：按当前 users.is_guest 近似回填；已被认领的游客任务无法
-- 还原创建时身份，会落到登录池。建议在新的一天执行迁移，或接受当天一次性偏差。
-- ============================================================

ALTER TABLE generation_tasks
    ADD COLUMN IF NOT EXISTS is_guest BOOLEAN NOT NULL DEFAULT false;

UPDATE generation_tasks t
SET is_guest = CASE
    -- 当前仍是游客：任务归游客池
    WHEN u.is_guest THEN true
    -- 已被登录用户认领的游客任务：通常创建于该账号注册之前
    WHEN t.created_at < u.created_at THEN true
    -- 账号注册之后创建的任务：登录用户池
    ELSE false
END
FROM users u
WHERE t.user_id = u.id;

CREATE INDEX IF NOT EXISTS idx_generation_tasks_created_guest
    ON generation_tasks (is_guest, created_at);
