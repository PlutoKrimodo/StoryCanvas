/**
 * BYOK 等敏感请求的错误摘要。
 *
 * 绝不能把 AxiosError 整体打印到控制台：它的 config.data 可能包含请求体中的
 * API Key。这里只提取 HTTP 状态与可读 message。
 */
interface ErrorPayload {
  response?: {
    status?: number
    data?: {
      message?: string
    }
  }
}

export function safeErrorMessage(error: unknown): string {
  const payload = error as ErrorPayload | null
  const status = payload?.response?.status
  const responseMessage = payload?.response?.data?.message
  const message = error instanceof Error ? error.message : ''

  if (status && responseMessage) {
    return `HTTP ${status}: ${responseMessage}`
  }
  if (responseMessage) {
    return responseMessage
  }
  if (message) {
    return message
  }
  return '未知错误'
}

export function logCredentialError(action: string, error: unknown): void {
  // 只记录安全摘要；不要展开 error/config/request。
  console.error(`[AI credential] ${action}: ${safeErrorMessage(error)}`)
}
