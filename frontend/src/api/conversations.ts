import client from './client'
import type { ApiResponse, PageResult } from '../types/api'
import type {
  BatchView,
  ConversationDetail,
  ConversationSummary,
  CreatePlanPayload,
  PlanResponse,
  StartBatchPayload,
  StoryboardPanel,
} from '../types/conversation'

export const conversationsApi = {
  list: (params?: { page?: number; limit?: number }) =>
    client.get<ApiResponse<PageResult<ConversationSummary>>>('/conversations', {
      params,
      silent: true,
    }),

  get: (conversationId: string) =>
    client.get<ApiResponse<ConversationDetail>>(`/conversations/${conversationId}`, {
      silent: true,
    }),

  create: (payload?: { title?: string; style?: string; size?: string }) =>
    client.post<ApiResponse<ConversationSummary>>('/conversations', payload ?? {}),

  remove: (conversationId: string) =>
    client.delete<ApiResponse<null>>(`/conversations/${conversationId}`),

  createPlan: (payload: CreatePlanPayload) =>
    // 分镜是一次真实 LLM 调用，覆盖全局 30s 超时，避免慢模型被前端提前中断
    client.post<ApiResponse<PlanResponse>>('/generation/plans', payload, {
      timeout: 60000,
    }),

  updatePlan: (batchId: string, panels: StoryboardPanel[]) =>
    client.put<ApiResponse<{ batch_id: string; panels: StoryboardPanel[]; status: string }>>(
      `/generation/plans/${batchId}`,
      { panels },
    ),

  startBatch: (batchId: string, payload: StartBatchPayload) =>
    client.post<ApiResponse<BatchView>>(
      `/generation/batches/${batchId}/start`,
      payload,
    ),

  getBatch: (batchId: string) =>
    client.get<ApiResponse<BatchView>>(`/generation/batches/${batchId}`, {
      silent: true,
    }),

  cancelBatch: (batchId: string) =>
    client.post<ApiResponse<BatchView>>(`/generation/batches/${batchId}/cancel`),

  regeneratePanel: (batchId: string, panelIndex: number) =>
    client.post<ApiResponse<BatchView>>(
      `/generation/batches/${batchId}/panels/${panelIndex}/regenerate`,
    ),
}
