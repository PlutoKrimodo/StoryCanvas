import { cleanup, render, screen } from '@testing-library/react'
import { MemoryRouter } from 'react-router-dom'
import type { ReactNode } from 'react'
import { afterEach, describe, expect, it, vi } from 'vitest'

import { useAuthStore } from '../../stores/authStore'
import AppLayout from './AppLayout'
import PageHeader from './PageHeader'
import Wordmark from './Wordmark'

// ResizeObserver / matchMedia 的 jsdom 兜底见 src/test/setup.ts

const guest = { isAuthenticated: false, user: null }
const signedIn = {
  isAuthenticated: true,
  user: { id: 'u1', username: '绘本画家', email: 'painter@example.com' },
}

const tree = (node: ReactNode, at = '/') =>
  render(<MemoryRouter initialEntries={[at]}>{node}</MemoryRouter>)

describe('共享布局（docs/17 §3.1）', () => {
  afterEach(() => {
    cleanup()
    useAuthStore.setState(guest)
  })

  it('Wordmark：11 个隐藏字母 + 一个可读标签', () => {
    tree(<Wordmark size="hero" tagline="把一段故事，画成一幅绘本插画" />)
    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByText('把一段故事，画成一幅绘本插画')).toBeInTheDocument()
  })

  it('hero：游客态只出登录/注册，不出顶栏导航', () => {
    tree(
      <AppLayout variant="hero">
        <div>generator-slot</div>
      </AppLayout>,
    )
    expect(screen.getByText('generator-slot')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /登\s*录/ })).toBeInTheDocument()
    // antd 会在两字中文按钮里插一个全角空格（注 册），故用正则匹配
    expect(screen.getByRole('button', { name: /注\s*册/ })).toBeInTheDocument()
    expect(screen.queryByRole('button', { name: '导航' })).not.toBeInTheDocument()
  })

  it('app：游客见登录/注册且无「我的绘本」，登录后补齐导航与账户菜单', () => {
    const { unmount } = tree(<AppLayout variant="app">page</AppLayout>, '/generate')
    expect(screen.getByRole('link', { name: 'AI 生成' })).toBeInTheDocument()
    expect(screen.queryByRole('link', { name: '我的绘本' })).not.toBeInTheDocument()
    unmount()

    useAuthStore.setState(signedIn)
    tree(<AppLayout variant="app">page</AppLayout>, '/books')
    expect(screen.getByRole('link', { name: '我的绘本' })).toBeInTheDocument()
    expect(screen.getByText('绘本画家')).toBeInTheDocument()
  })

  it('auth：登录页出品牌与返回首页链接', () => {
    tree(
      <AppLayout variant="auth">
        <div>login-card</div>
      </AppLayout>,
      '/login',
    )
    expect(screen.getByText('login-card')).toBeInTheDocument()
    expect(screen.getByRole('link', { name: '返回体验首页' })).toBeInTheDocument()
  })

  it('PageHeader：标题/描述/操作区齐全，onBack 才出返回按钮', () => {
    const onBack = vi.fn()
    tree(
      <PageHeader title="我的绘本" description="共 3 本" extra={<button>新建绘本</button>} onBack={onBack} backLabel="回首页" />,
    )
    expect(screen.getByText('我的绘本')).toBeInTheDocument()
    expect(screen.getByText('共 3 本')).toBeInTheDocument()
    screen.getByRole('button', { name: /回首页/ }).click()
    expect(onBack).toHaveBeenCalledTimes(1)

    cleanup()
    tree(<PageHeader title="无返回" />)
    expect(screen.queryByRole('button', { name: /回首页/ })).not.toBeInTheDocument()
  })
})
