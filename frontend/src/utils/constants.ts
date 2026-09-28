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
