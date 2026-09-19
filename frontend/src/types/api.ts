/** 后端统一响应结构：{ code, message, data }，失败时 data 为 null */
export interface ApiResponse<T = unknown> {
  code: number
  message: string
  data: T | null
  errors?: Array<{ field: string; message: string }>
}

/** 分页结果 */
export interface PageResult<T> {
  items: T[]
  total: number
  page: number
  limit: number
}
