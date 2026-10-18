import { cleanup, fireEvent, screen } from '@testing-library/react'
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

  it('Login：邮箱/用户名双卡片切换，邮箱模式保留格式校验', async () => {
    const { container } = renderWithTheme(<Login />, { at: '/login' })
    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByRole('link', { name: '立即注册' })).toBeInTheDocument()

    // 默认邮箱模式
    expect(screen.getByRole('tab', { name: /邮箱登录/ })).toHaveAttribute(
      'aria-selected',
      'true',
    )
    expect(screen.getByLabelText('邮箱')).toBeInTheDocument()

    // 邮箱格式校验仍然生效
    fireEvent.change(screen.getByLabelText('邮箱'), { target: { value: 'not-an-email' } })
    fireEvent.change(screen.getByLabelText('密码'), { target: { value: 'Password1' } })
    fireEvent.click(screen.getByRole('button', { name: /登\s*录/ }))
    expect(await screen.findByText('邮箱格式不正确')).toBeInTheDocument()

    // 切到用户名模式：不再使用邮箱字段
    fireEvent.click(screen.getByRole('tab', { name: /用户名登录/ }))
    expect(screen.getByLabelText('用户名')).toBeInTheDocument()
    expect(screen.queryByLabelText('邮箱')).not.toBeInTheDocument()
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
    expect(screen.getByText('painter@example.com')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /退出登录/ })).toBeInTheDocument()
    expect(container.querySelector('.bg-gray-50')).toBeNull()
  })
})
