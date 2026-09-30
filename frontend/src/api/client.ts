import axios, { AxiosError, AxiosResponse, InternalAxiosRequestConfig } from 'axios'
import { message } from 'antd'

import { useAuthStore } from '../stores/authStore'
import type { ApiResponse } from '../types/api'
import type { GuestSession, RefreshResponse } from '../types/auth'

/**
 * 扩展 axios 请求配置。
 *
 * `silent: true` 表示该请求失败时不弹全局 toast，交给调用方自行处理。
 * 用于**后台轮询**这类「失败会被自动重试、且频率高」的请求：
 * 否则轮询每失败一次就弹一次提示，会连续弹窗刷屏。
 */
declare module 'axios' {
  export interface AxiosRequestConfig {
    silent?: boolean
  }
}

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
    const { accessToken, guestToken } = useAuthStore.getState()
    // 已登录用 Access Token；未登录（游客）用游客 Token
    const token = accessToken ?? guestToken
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => Promise.reject(error),
)

/** 单飞（single-flight）：并发 401 时只触发一次刷新 */
let refreshPromise: Promise<string> | null = null
/** 单飞（single-flight）：并发触发游客会话创建时只请求一次 */
let guestPromise: Promise<string> | null = null

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

/** 获取（或创建）游客会话 Token，供未登录用户调用生成等受保护接口 */
export async function ensureGuestToken(): Promise<string> {
  const { guestToken } = useAuthStore.getState()
  if (guestToken) {
    return guestToken
  }

  if (!guestPromise) {
    guestPromise = axios
      .post<ApiResponse<GuestSession>>(`${BASE_URL}/auth/guest`)
      .then(({ data }) => {
        if (!data.data) {
          throw new Error(data.message || '创建游客会话失败')
        }
        useAuthStore.getState().setGuest(data.data)
        return data.data.guest_token
      })
      .finally(() => {
        guestPromise = null
      })
  }
  return guestPromise
}

/**
 * 游客会话重建的最小间隔（毫秒）。
 *
 * 后端每次 `POST /auth/guest` 都会**新建一个影子游客用户**，因此这里必须限流，
 * 否则 Token 持续失效时（高频轮询场景）会在 users 表里批量堆出游客账号。
 */
const GUEST_RESESSION_COOLDOWN_MS = 60_000
/** 上次成功重建游客会话的时间戳 */
let lastGuestSessionAt = 0
/** 单飞（single-flight）：并发 401 时只重建一次游客会话 */
let guestResessionPromise: Promise<string> | null = null

/**
 * 重建游客会话（带单飞与冷却）。
 *
 * 与 `ensureGuestToken` 的区别：本函数会先丢弃已确认失效的旧 Token，
 * 并受冷却窗口限制，冷却期内直接失败（不调后端），避免高频创建游客账号。
 */
async function resessionGuest(): Promise<string> {
  if (!guestResessionPromise) {
    guestResessionPromise = (async () => {
      // 无论是否重建，都先把已失效的 Token 清掉，避免继续拿它发请求
      useAuthStore.getState().setGuest(null)
      if (Date.now() - lastGuestSessionAt < GUEST_RESESSION_COOLDOWN_MS) {
        throw new Error('游客会话重建过于频繁，本次跳过')
      }
      const token = await ensureGuestToken()
      lastGuestSessionAt = Date.now()
      return token
    })().finally(() => {
      guestResessionPromise = null
    })
  }
  return guestResessionPromise
}

client.interceptors.response.use(
  (response: AxiosResponse) => response,
  async (error: AxiosError<ApiResponse>) => {
    const original = error.config as (InternalAxiosRequestConfig & { _retry?: boolean }) | undefined

    if (error.response?.status === 401 && original && !original._retry) {
      original._retry = true
      const { isAuthenticated, refreshToken, guestToken } = useAuthStore.getState()

      if (isAuthenticated || refreshToken) {
        // 真实用户：刷新 Access Token 后重试
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

      if (guestToken) {
        // 静默请求（后台轮询）不重建游客会话：它频率高且非用户触发，
        // 若每次 401 都调 POST /auth/guest，后端会不断新建影子游客用户。
        // 这里只丢弃失效 Token，等下一次用户操作（如点击生成）时自然重建。
        if (original.silent) {
          useAuthStore.getState().setGuest(null)
          return Promise.reject(error)
        }

        // 用户触发的请求：Token 可能已过期 / 被撤销，重建会话后重试一次。
        // 重建受单飞与冷却保护，避免 401 风暴批量创建游客账号。
        try {
          const token = await resessionGuest()
          original.headers.Authorization = `Bearer ${token}`
          return client(original)
        } catch {
          return Promise.reject(error)
        }
      }
    }

    // 静默请求（如后台轮询）不弹全局提示：它会被自动重试，弹窗会连续刷屏。
    // 注意：此处只提示 message，禁止打印请求体（可能含密码）
    if (!original?.silent) {
      message.error(error.response?.data?.message || '请求失败')
    }
    return Promise.reject(error)
  },
)

export default client
