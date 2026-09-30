import { create } from 'zustand'
import { persist } from 'zustand/middleware'

import type { AuthResponse, GuestSession, User } from '../types/auth'

interface AuthState {
  user: User | null
  accessToken: string | null
  refreshToken: string | null
  isAuthenticated: boolean

  /** Guest-first：游客会话（未登录用户先体验生成） */
  guestId: string | null
  guestToken: string | null
  guestQuota: number

  login: (response: AuthResponse) => void
  logout: () => void
  updateUser: (user: Partial<User>) => void
  setGuest: (session: GuestSession | null) => void
}

export const useAuthStore = create<AuthState>()(
  persist(
    (set) => ({
      user: null,
      accessToken: null,
      refreshToken: null,
      isAuthenticated: false,

      guestId: null,
      guestToken: null,
      guestQuota: 1,

      login: (response) =>
        set({
          user: response.user,
          accessToken: response.access_token,
          refreshToken: response.refresh_token,
          isAuthenticated: true,
          // 登录后清空游客身份：其资源已由后端「认领」转移到真实用户
          guestId: null,
          guestToken: null,
        }),

      logout: () =>
        set({
          user: null,
          accessToken: null,
          refreshToken: null,
          isAuthenticated: false,
        }),

      updateUser: (userData) =>
        set((state) => ({
          user: state.user ? { ...state.user, ...userData } : null,
        })),

      setGuest: (session) =>
        set(
          session
            ? {
                guestId: session.guest_id,
                guestToken: session.guest_token,
                guestQuota: session.quota ?? 1,
              }
            : { guestId: null, guestToken: null },
        ),
    }),
    {
      name: 'auth-storage',
      // 仅持久化 Token 与基础用户信息；禁止存放密码或任何服务端密钥
      partialize: (state) => ({
        user: state.user,
        accessToken: state.accessToken,
        refreshToken: state.refreshToken,
        isAuthenticated: state.isAuthenticated,
        guestId: state.guestId,
        guestToken: state.guestToken,
        guestQuota: state.guestQuota,
      }),
    },
  ),
)
