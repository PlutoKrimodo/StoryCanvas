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

export interface GenerationRequest {
  text: string
  book_id?: string
  page_id?: string
  style: ArtStyle
  parameters?: Record<string, unknown>
}

export interface GenerationResponse {
  task_id: string
  status: TaskStatus
  created_at: string
}

export interface TaskResult {
  task_id: string
  status: TaskStatus
  original_text: string
  prompt: string
  parsed_data?: ParsedData | null
  result?: { image_url: string; width: number; height: number } | null
  error_message?: string | null
  created_at: string
  completed_at?: string | null
}
