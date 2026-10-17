import client from './client'
import type { ApiResponse } from '../types/api'
import type { BookPage, SavePageRequest } from '../types/page'

export const pagesApi = {
  /** 保存到绘本（创建/更新页并绑定生成图） */
  save: (bookId: string, payload: SavePageRequest) =>
    client.post<ApiResponse<BookPage>>(`/books/${bookId}/pages`, payload),

  /** 整组图片按顺序写入连续页 */
  saveBatch: (bookId: string, imageIds: string[]) =>
    client.post<ApiResponse<{ items: BookPage[] }>>(`/books/${bookId}/pages/batch`, {
      image_ids: imageIds,
    }),

  /** 读取绘本页列表（按页码升序，供 PDF 导出） */
  list: (bookId: string) => client.get<ApiResponse<BookPage[]>>(`/books/${bookId}/pages`),
}
