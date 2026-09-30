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

  /**
   * 查询任务状态（前端轮询用）。
   *
   * 标记 `silent`：轮询失败由 useGeneration 自行记录并继续重试，
   * 不走全局 toast，避免瞬时网络抖动导致连续弹窗。
   */
  getTask: (taskId: string) =>
    client.get<ApiResponse<TaskResult>>(`/generation/${taskId}`, { silent: true }),

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
