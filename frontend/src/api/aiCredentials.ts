import client from './client'
import type { ApiResponse } from '../types/api'
import type {
  AiCredentialOverview,
  CredentialCapability,
  CredentialCatalog,
  CredentialTestRequest,
  CredentialTestResult,
  SaveCredentialPayload,
  UserAiCredential,
} from '../types/aiCredentials'

export const aiCredentialsApi = {
  // silent：失败时由页面展示可执行提示，避免轮询/页面初始化反复弹全局 toast
  getOverview: () =>
    client.get<ApiResponse<AiCredentialOverview>>('/users/me/ai-credentials', {
      silent: true,
    }),

  getProviders: () =>
    client.get<ApiResponse<CredentialCatalog>>('/users/me/ai-credentials/providers', {
      silent: true,
    }),

  save: (capability: CredentialCapability, payload: SaveCredentialPayload) =>
    client.put<ApiResponse<UserAiCredential>>(
      `/users/me/ai-credentials/${capability}`,
      payload,
      { silent: true },
    ),

  remove: (capability: CredentialCapability) =>
    client.delete<ApiResponse<null>>(`/users/me/ai-credentials/${capability}`, {
      silent: true,
    }),

  test: (payload: CredentialTestRequest) =>
    client.post<ApiResponse<CredentialTestResult>>(
      '/users/me/ai-credentials/test',
      payload,
      { silent: true },
    ),
}
