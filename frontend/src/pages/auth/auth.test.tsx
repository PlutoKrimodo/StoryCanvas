import { cleanup, screen } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'

import { useAuthStore } from '../../stores/authStore'
import Login from './Login'
import Register from './Register'
import Profile from '../profile/Profile'
import { renderWithTheme } from '../../test/renderWithTheme'

const signedIn = {
  isAuthenticated: true,
  user: {
    id: 'u1',
    username: '绘本画家',
    email: 'painter@example.com',
    created_at: '2026-10-01T08:00:00.000Z',
  },
}

describe('认证页与个人中心接入新体系（docs/17 Step 8）', () => {
  afterEach(() => {
    cleanup()
    useAuthStore.setState({ isAuthenticated: false, user: null })
  })

  it('Login：auth 形态出品牌与注册入口，无冷色渐变页底', () => {
    const { container } = renderWithTheme(<Login />, { at: '/login' })
    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByRole('link', { name: '立即注册' })).toBeInTheDocument()
    expect(screen.getByLabelText('邮箱')).toBeInTheDocument()
    expect(container.querySelector('.bg-gradient-to-br')).toBeNull()
  })

  it('Register：四个字段齐全并可回到登录', () => {
    const { container } = renderWithTheme(<Register />, { at: '/register' })
    ;['用户名', '邮箱', '密码', '确认密码'].forEach((label) => {
      expect(screen.getByLabelText(label)).toBeInTheDocument()
    })
    expect(screen.getByRole('link', { name: '返回登录' })).toBeInTheDocument()
    expect(container.querySelector('.bg-gradient-to-br')).toBeNull()
  })

  it('Profile：app 形态 + PageHeader，账号信息可读', () => {
    useAuthStore.setState(signedIn)
    const { container } = renderWithTheme(<Profile />, { at: '/profile' })
    expect(screen.getByRole('heading', { name: '个人中心' })).toBeInTheDocument()
    expect(screen.getByRole('link', { name: '我的绘本' })).toBeInTheDocument()
    expect(screen.getByText('painter@example.com')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /退出登录/ })).toBeInTheDocument()
    expect(container.querySelector('.bg-gray-50')).toBeNull()
  })
})
