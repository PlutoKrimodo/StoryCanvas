-- ============================================================
-- StoryCanvas 增量迁移：book_pages.image_id（v2.2「保存到绘本」）
--
-- 背景：init.sql v2.2 为 book_pages 增加了 image_id（该页绑定的生成图，
--       用于轻量多页 PDF 导出）。早期创建的数据库缺少该列，表现为：
--         ERROR: column "image_id" of relation "book_pages" does not exist
--       即「保存到绘本」接口返回 500、绘本页列表查询报错。
--
-- 全新数据库请直接使用 sql/init.sql（已包含该列），无需执行本脚本。
--
-- 用法：
--   PGPASSWORD=你的密码 psql -h localhost -U storycanvas -d storycanvas \
--       -f sql/migrate_page_image.sql
-- ============================================================

ALTER TABLE book_pages
    ADD COLUMN IF NOT EXISTS image_id UUID;

CREATE INDEX IF NOT EXISTS idx_book_pages_image_id ON book_pages(image_id);

-- PostgreSQL 不支持 ADD CONSTRAINT IF NOT EXISTS，用 DO 块保证幂等
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'fk_book_pages_image'
    ) THEN
        ALTER TABLE book_pages
            ADD CONSTRAINT fk_book_pages_image
            FOREIGN KEY (image_id) REFERENCES generated_images(id) ON DELETE SET NULL;
    END IF;
END $$;

COMMENT ON COLUMN book_pages.image_id IS '该页绑定的生成图（用于 PDF 导出），图片删除时置空';
