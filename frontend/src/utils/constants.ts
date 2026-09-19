import { ArtStyle } from '../types/generation'

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
