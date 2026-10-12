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

/**
 * 艺术风格视觉元数据（docs/17 §4.2）。palette 直接取自
 * `ai-service/app/prompts/styles/*.py` 的 `palette` 字段，保证「色卡—提示词」
 * 同源；后端改色时这里要同步（additive 导出，不改 ART_STYLE_OPTIONS）。
 */
export const ART_STYLE_META: Record<
  ArtStyle,
  { label: string; description: string; palette: [string, string, string, string] }
> = {
  [ArtStyle.CARTOON]: {
    label: '卡通风格',
    description: '粗描边·高饱和',
    palette: ['#FF6B6B', '#FFD93D', '#6BCB77', '#4D96FF'],
  },
  [ArtStyle.WATERCOLOR]: {
    label: '水彩风格',
    description: '通透晕染·柔和边',
    palette: ['#FFE3E3', '#E3F2FD', '#E8F5E9', '#FFF8E1'],
  },
  [ArtStyle.OIL_PAINTING]: {
    label: '油画风格',
    description: '厚涂笔触·肌理',
    palette: ['#8D6E63', '#C62828', '#F9A825', '#1565C0'],
  },
  [ArtStyle.ANIME]: {
    label: '动漫风格',
    description: '赛璐璐·清亮高光',
    palette: ['#90CAF9', '#F48FB1', '#FFF59D', '#B39DDB'],
  },
  [ArtStyle.PENCIL_SKETCH]: {
    label: '铅笔素描',
    description: '单色线条·明暗排线',
    palette: ['#212121', '#616161', '#BDBDBD', '#F5F5F5'],
  },
}

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
