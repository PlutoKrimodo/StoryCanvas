import axios, { AxiosError, AxiosResponse, InternalAxiosRequestConfig } from 'axios'
import { message } from 'antd'

import { useAuthStore } from '../stores/authStore'
import type { ApiResponse } from '../types/api'
import type { RefreshResponse } from '../types/auth'

/**
 * 使用相对路径，开发环境由 Vite 代理转发到后端（见 vite.config.ts），
 * 生产环境由 Nginx 反向代理，避免硬编码地址导致代理失效。
 */
export const BASE_URL = import.meta.env.VITE_API_BASE_URL || '/api/v1'

const client = axios.create({
  baseURL: BASE_URL,
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json',
  },
})

client.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    const { accessToken } = useAuthStore.getState()
    if (accessToken) {
      config.headers.Authorization = `Bearer ${accessToken}`
    }
    return config
  },
  (error) => Promise.reject(error),
)

/** 单飞（single-flight）：并发 401 时只触发一次刷新 */
let refreshPromise: Promise<string> | null = null

async function refreshAccessToken(): Promise<string> {
  const { refreshToken } = useAuthStore.getState()
  if (!refreshToken) {
    throw new Error('缺少 Refresh Token')
  }

  // 用裸 axios 发起，避免再次进入本拦截器造成递归
  const { data } = await axios.post<ApiResponse<RefreshResponse>>(
    `${BASE_URL}/auth/refresh`,
    { refresh_token: refreshToken },
  )

  if (!data.data) {
    throw new Error('刷新响应缺少 data')
  }

  const { user } = useAuthStore.getState()
  useAuthStore.getState().login({
    user: data.data.user ?? user!,
    access_token: data.data.access_token,
    refresh_token: data.data.refresh_token,
  })

  return data.data.access_token
}

client.interceptors.response.use(
  (response: AxiosResponse) => response,
  async (error: AxiosError<ApiResponse>) => {
    const original = error.config as (InternalAxiosRequestConfig & { _retry?: boolean }) | undefined

    if (error.response?.status === 401 && original && !original._retry) {
      original._retry = true
      try {
        if (!refreshPromise) {
          refreshPromise = refreshAccessToken().finally(() => {
            refreshPromise = null
          })
        }
        const token = await refreshPromise
        original.headers.Authorization = `Bearer ${token}`
        return client(original)
      } catch {
        useAuthStore.getState().logout()
        if (window.location.pathname !== '/login') {
          window.location.href = '/login'
        }
        return Promise.reject(error)
      }
    }

    // 注意：此处只提示 message，禁止打印请求体（可能含密码）
    message.error(error.response?.data?.message || '请求失败')
    return Promise.reject(error)
  },
)

export default client
