/** 绘本页容器（与后端 BookPage.toJson 对齐） */
export interface BookPage {
  id: string
  book_id: string
  page_number: number
  title?: string | null
  thumbnail?: string | null
  image_id?: string | null
  image_url?: string | null
  created_at?: string | null
  updated_at?: string | null
}

/** 「保存到绘本」请求 */
export interface SavePageRequest {
  page_number: number
  title?: string
  image_id: string
}
