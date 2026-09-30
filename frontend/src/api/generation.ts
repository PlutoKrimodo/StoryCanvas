import client from './client'
import type { ApiResponse, PageResult } from '../types/api'
import type {
  GenerationHistoryQuery,
  GenerationRequest,
  GenerationResponse,
  RegenerateRequest,
  TaskResult,
} from '../types/generation'

export const generationApi = {
  /** 创建生成任务 */
  createTask: (payload: GenerationRequest) =>
    client.post<ApiResponse<GenerationResponse>>('/generation', payload),

  /** 查询任务状态（前端轮询用） */
  getTask: (taskId: string) =>
    client.get<ApiResponse<TaskResult>>(`/generation/${taskId}`),

  /** 生成历史（分页 + 可选按绘本筛选） */
  getHistory: (params?: GenerationHistoryQuery) =>
    client.get<ApiResponse<PageResult<TaskResult>>>('/generation/history', { params }),

  /** 取消任务 */
  cancel: (taskId: string) =>
    client.post<ApiResponse<null>>(`/generation/${taskId}/cancel`),

  /** 基于已有任务重新生成 */
  regenerate: (taskId: string, payload?: RegenerateRequest) =>
    client.post<ApiResponse<GenerationResponse>>(
      `/generation/${taskId}/regenerate`,
      payload ?? {},
    ),
}
