import { useCallback, useState } from 'react'
import { useNavigate } from 'react-router-dom'

import { authApi } from '../api/auth'
import { useAuthStore } from '../stores/authStore'
import type { LoginRequest, RegisterRequest } from '../types/auth'

export function useAuth() {
  const navigate = useNavigate()
  const [loading, setLoading] = useState(false)
  const { user, isAuthenticated, login, logout } = useAuthStore()

  const loginWithPassword = useCallback(
    async (payload: LoginRequest) => {
      setLoading(true)
      try {
        const { data } = await authApi.login({
          ...payload,
          // 携带游客身份，登录后由后端认领游客期间生成的图片
          guest_id: useAuthStore.getState().guestId ?? undefined,
        })
        if (!data.data) {
          throw new Error(data.message || '登录失败')
        }
        login(data.data)
        navigate('/', { replace: true })
      } finally {
        setLoading(false)
      }
    },
    [login, navigate],
  )

  const register = useCallback(async (payload: RegisterRequest) => {
    setLoading(true)
    try {
      const { data } = await authApi.register({
        ...payload,
        // 注册后由后端认领游客期间生成的图片
        guest_id: useAuthStore.getState().guestId ?? undefined,
      })
      if (!data.data) {
        throw new Error(data.message || '注册失败')
      }
      return data.data
    } finally {
      setLoading(false)
    }
  }, [])

  const signOut = useCallback(async () => {
    try {
      await authApi.logout()
    } catch {
      // 退出接口失败也要清理本地状态
    }
    logout()
    navigate('/login', { replace: true })
  }, [logout, navigate])

  return { user, isAuthenticated, loading, loginWithPassword, register, signOut }
}
