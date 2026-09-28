import client from './client'
import type { ApiResponse, PageResult } from '../types/api'
import type { Book, BookListQuery, CreateBookRequest, UpdateBookRequest } from '../types/book'

export const booksApi = {
  /** 创建绘本 */
  create: (payload: CreateBookRequest) => client.post<ApiResponse<Book>>('/books', payload),

  /** 获取当前用户的绘本列表（分页 + 状态筛选） */
  listMine: (params?: BookListQuery) =>
    client.get<ApiResponse<PageResult<Book>>>('/users/me/books', { params }),

  /** 获取绘本详情 */
  get: (bookId: string) => client.get<ApiResponse<Book>>(`/books/${bookId}`),

  /** 更新绘本 */
  update: (bookId: string, payload: UpdateBookRequest) =>
    client.put<ApiResponse<Book>>(`/books/${bookId}`, payload),

  /** 删除绘本 */
  remove: (bookId: string) => client.delete<ApiResponse<null>>(`/books/${bookId}`),
}
