import { ArtStyle } from '../types/generation'
import type { BookStatus } from '../types/book'

/** 艺术风格可选项（与后端 ArtStyle 枚举保持一致） */
export const ART_STYLE_OPTIONS = [
  { value: ArtStyle.CARTOON, label: '卡通风格' },
  { value: ArtStyle.WATERCOLOR, label: '水彩风格' },
  { value: ArtStyle.OIL_PAINTING, label: '油画风格' },
  { value: ArtStyle.ANIME, label: '动漫风格' },
  { value: ArtStyle.PENCIL_SKETCH, label: '铅笔素描' },
] as const

/** 故事文本长度上限（与后端校验一致） */
export const STORY_TEXT_MAX_LENGTH = 2000

/** 生成任务状态文案 */
export const TASK_STATUS_TEXT: Record<string, string> = {
  pending: '等待中',
  processing: '生成中',
  completed: '已完成',
  failed: '生成失败',
  cancelled: '已取消',
}

/** 生成任务状态标签配色（Ant Design Tag color） */
export const TASK_STATUS_COLOR: Record<string, string> = {
  pending: 'processing',
  processing: 'processing',
  completed: 'success',
  failed: 'error',
  cancelled: 'default',
}

/**
 * 生成图尺寸可选项。
 *
 * 真实厂商（豆包 Seedream）要求图片面积 ≥ 921600 像素，低于该值会返回
 * 400 InvalidParameter（实测 768x768 / 1024x768 / 768x1024 均被拒绝，
 * 1024x1024 / 1280x768 / 768x1280 可用），因此只保留合规档位。
 */
export const IMAGE_SIZE_OPTIONS = [
  { value: '1024x1024', label: '1024 × 1024（方形·推荐）' },
  { value: '1280x768', label: '1280 × 768（横向）' },
  { value: '768x1280', label: '768 × 1280（纵向·绘本页）' },
] as const

/** 单本绘本页数上限（与后端 SavePageRequest 校验、PDF 导出上限保持一致） */
export const MAX_PAGES_PER_BOOK = 50

/**
 * 游客免费生成额度（前端仅用于展示；真正的额度判断与 429 以后端为准，
 * 后端默认 GUEST_GENERATION_QUOTA=1，见 .env.example）
 */
export const GUEST_GENERATION_LIMIT = 1

/** 绘本状态可选项（与后端 books.status 枚举保持一致） */
export const BOOK_STATUS_OPTIONS: Array<{ value: BookStatus; label: string }> = [
  { value: 'draft', label: '草稿' },
  { value: 'published', label: '已发布' },
  { value: 'archived', label: '已归档' },
]

/** 绘本状态文案 */
export const BOOK_STATUS_TEXT: Record<BookStatus, string> = {
  draft: '草稿',
  published: '已发布',
  archived: '已归档',
}

/** 绘本状态标签配色（Ant Design Tag color） */
export const BOOK_STATUS_COLOR: Record<BookStatus, string> = {
  draft: 'default',
  published: 'green',
  archived: 'orange',
}
