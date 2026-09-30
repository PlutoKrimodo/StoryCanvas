import client, { BASE_URL } from './client'
import type { ApiResponse, PageResult } from '../types/api'
import type { GeneratedImage, ImageListQuery } from '../types/image'

export const imagesApi = {
  /** 图片详情 */
  get: (imageId: string) => client.get<ApiResponse<GeneratedImage>>(`/images/${imageId}`),

  /** 当前用户图片列表 */
  list: (params?: ImageListQuery) =>
    client.get<ApiResponse<PageResult<GeneratedImage>>>('/images', { params }),

  /** 删除图片 */
  remove: (imageId: string) => client.delete<ApiResponse<null>>(`/images/${imageId}`),

  /**
   * 直链（公开，供 `<img src>` 使用）。
   * 图片 ID 为 UUID，后端按"不可枚举即不可达"处理。
   */
  rawUrl: (imageId: string) => `${BASE_URL}/images/${imageId}/raw`,

  /** 带 JWT 拉取二进制（下载 / PDF 导出用） */
  fetchBlob: async (imageId: string): Promise<Blob> => {
    const response = await client.get(`/images/${imageId}/raw`, { responseType: 'blob' })
    return response.data as Blob
  },

  /** 下载图片（走鉴权接口，前端触发浏览器保存） */
  download: async (imageId: string, filename?: string): Promise<void> => {
    const response = await client.get(`/images/${imageId}/download`, { responseType: 'blob' })
    const blob = response.data as Blob
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = filename ?? `storycanvas-${imageId}.png`
    document.body.appendChild(link)
    link.click()
    link.remove()
    URL.revokeObjectURL(url)
  },
}
