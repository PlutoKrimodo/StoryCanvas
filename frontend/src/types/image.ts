/** AI 生成图（与后端 GeneratedImage.toJson 对齐） */
export interface GeneratedImage {
  id: string
  task_id: string
  file_path: string
  image_url: string
  file_size?: number | null
  width?: number | null
  height?: number | null
  format?: string | null
  prompt_used?: string | null
  is_favorite?: boolean
  created_at?: string | null
}

export interface ImageListQuery {
  page?: number
  limit?: number
  book_id?: string
}
