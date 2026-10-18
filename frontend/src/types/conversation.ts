import type { ArtStyle } from './generation'

export interface StoryboardPanel {
  index: number
  text: string
}

export interface StoryboardCharacter {
  name: string
  appearance?: string
}

export interface StoryboardConsistency {
  characters?: StoryboardCharacter[]
  palette?: string
  notes?: string
}

export interface PlanResponse {
  plan_id: string
  batch_id: string
  conversation_id: string | null
  message_id: string | null
  title: string
  style: ArtStyle
  size: string
  panel_count: number
  panels: StoryboardPanel[]
  consistency?: StoryboardConsistency
  consistency_prompt?: string
  negative_prompt?: string
  status: string
  quota_exempt: boolean
}

export interface BatchItem {
  task_id: string
  panel_index: number | null
  panel_text?: string | null
  status: string
  image_id?: string | null
  image_url?: string | null
  prompt?: string
  model_used?: string | null
  error_message?: string | null
}

export interface BatchView {
  batch_id: string
  conversation_id?: string | null
  message_id?: string | null
  status: string
  style: ArtStyle
  size?: string | null
  seed?: number | null
  panel_count: number
  panels: StoryboardPanel[]
  consistency?: StoryboardConsistency
  consistency_prompt?: string | null
  negative_prompt?: string | null
  items: BatchItem[]
  error_message?: string | null
}

export interface ChatPayload {
  batch_id?: string
  panels?: StoryboardPanel[]
  consistency?: StoryboardConsistency
  status?: string
  images?: BatchItem[]
  seed?: number | null
}

export interface ChatMessage {
  id: string
  role: 'user' | 'assistant'
  messageType: 'text' | 'storyboard' | 'images'
  content?: string | null
  payload: ChatPayload
  createdAt: string
  /** 仅前端本地消息，用于乐观渲染 */
  local?: boolean
  /** 前端占位消息：等待分镜接口返回 */
  pending?: boolean
}

export interface ConversationSummary {
  id: string
  title?: string | null
  style?: string | null
  size?: string | null
  created_at: string
  updated_at: string
}

export interface ConversationDetail {
  conversation: ConversationSummary
  messages: Array<{
    id: string
    conversation_id: string
    role: 'user' | 'assistant'
    message_type: 'text' | 'storyboard' | 'images'
    content?: string | null
    payload: ChatPayload
    created_at: string
  }>
}

export interface CreatePlanPayload {
  conversation_id?: string
  text: string
  image_count: number
  style: ArtStyle
  size: string
}

export interface StartBatchPayload {
  seed?: number
  book_id?: string
  style?: ArtStyle
  size?: string
}
