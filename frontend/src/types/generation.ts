export enum TaskStatus {
  PENDING = 'pending',
  PROCESSING = 'processing',
  COMPLETED = 'completed',
  FAILED = 'failed',
  CANCELLED = 'cancelled',
}

export enum ArtStyle {
  CARTOON = 'cartoon',
  WATERCOLOR = 'watercolor',
  OIL_PAINTING = 'oil_painting',
  ANIME = 'anime',
  PENCIL_SKETCH = 'pencil_sketch',
}

export interface ParsedCharacter {
  name: string
  description?: string
  appearance?: string
  position?: string
}

export interface ParsedScene {
  location?: string
  time?: string
  weather?: string
  mood?: string
}

export interface ParsedData {
  characters: ParsedCharacter[]
  scene: ParsedScene
  objects: string[]
  actions: string[]
  emotions: string[]
  style: Record<string, unknown>
}

/** 生成参数（可选，透传给 AI 服务） */
export interface GenerationParameters {
  /** 图片边长，如 "1024x1024"；也可用 width/height 指定 */
  size?: string
  width?: number
  height?: number
  seed?: number
}

/** 创建生成任务请求 */
export interface GenerationRequest {
  text: string
  style: ArtStyle
  /** 可选：直接绑定到某个绘本（不传即为「先生成、后保存」的解耦流程） */
  book_id?: string
  page_id?: string
  parameters?: GenerationParameters
}

/** 创建任务响应 */
export interface GenerationResponse {
  task_id: string
  status: TaskStatus
  created_at: string
}

/** 任务结果中携带的生成图信息 */
export interface GenerationResultImage {
  image_id?: string | null
  image_url?: string | null
  width?: number | null
  height?: number | null
}

/** 任务详情（与后端 GenerationTask.toJson 对齐） */
export interface TaskResult {
  task_id: string
  user_id?: string
  book_id?: string | null
  page_id?: string | null
  status: TaskStatus
  original_text?: string | null
  prompt: string
  parsed_data?: ParsedData | null
  result?: GenerationResultImage | null
  error_message?: string | null
  model_used?: string | null
  /** 生成参数（含 style / size / seed），用于历史复用 */
  parameters?: Record<string, unknown> | null
  created_at: string
  updated_at?: string | null
  completed_at?: string | null
}

export interface RegenerateRequest {
  parameters?: GenerationParameters
}

export interface GenerationHistoryQuery {
  page?: number
  limit?: number
  book_id?: string
}
