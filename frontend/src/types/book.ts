export type BookStatus = 'draft' | 'published' | 'archived'

export interface Book {
  id: string
  user_id: string
  title: string
  description?: string | null
  cover_image?: string | null
  status: BookStatus
  created_at: string
  updated_at: string
}

export interface CreateBookRequest {
  title: string
  description?: string
  status?: BookStatus
}

export interface UpdateBookRequest {
  title: string
  description?: string
  status?: BookStatus
}

export interface BookListQuery {
  page?: number
  limit?: number
  status?: BookStatus
}
