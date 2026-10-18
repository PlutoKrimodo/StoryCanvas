import { cleanup, fireEvent, screen } from '@testing-library/react'
import { afterEach, describe, expect, it, vi } from 'vitest'

import Home from './Home'
import { clearChatSessionCache } from '../../hooks/useChatGeneration'
import { renderWithTheme } from '../../test/renderWithTheme'
import { useAuthStore } from '../../stores/authStore'

vi.mock('../../api/conversations', () => ({
  conversationsApi: {
    list: () => Promise.resolve({ data: { data: { items: [], total: 0 } } }),
    get: () => Promise.resolve({ data: { data: null } }),
    getBatch: () => Promise.resolve({ data: { data: null } }),
    createPlan: () => Promise.resolve({ data: { data: null } }),
    updatePlan: () => Promise.resolve({ data: { data: null } }),
    startBatch: () => Promise.resolve({ data: { data: null } }),
    cancelBatch: () => Promise.resolve({ data: { data: null } }),
    regeneratePanel: () => Promise.resolve({ data: { data: null } }),
    remove: () => Promise.resolve({ data: { data: null } }),
  },
}))

vi.mock('../../api/aiCredentials', () => ({
  aiCredentialsApi: {
    getOverview: () =>
      Promise.resolve({
        data: {
          data: {
            byok_enabled: true,
            encryption_ready: true,
            quota_exempt: false,
            credentials: { llm: null, image: null },
          },
        },
      }),
    getProviders: () => Promise.resolve({ data: { data: null } }),
    save: () => Promise.resolve({ data: { data: null } }),
    remove: () => Promise.resolve({ data: { data: null } }),
    test: () => Promise.resolve({ data: { data: null } }),
  },
}))

const signedIn = {
  isAuthenticated: true,
  user: { id: 'u1', username: '绘本画家', email: 'painter@example.com' },
}

describe('统一体验首页', () => {
  afterEach(() => {
    cleanup()
    clearChatSessionCache()
    useAuthStore.setState({ isAuthenticated: false, user: null })
  })

  it('游客：只保留大 LOGO + 对话框，旧 hero 文案与入口已移除', () => {
    renderWithTheme(<Home />)

    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByRole('textbox')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: '发送' })).toBeDisabled()

    // 旧首页元素与登录用户专属入口全部移除
    expect(screen.queryByText('把一段故事，画成一幅绘本插画')).not.toBeInTheDocument()
    expect(screen.queryByRole('button', { name: /开始创作/ })).not.toBeInTheDocument()
    expect(screen.queryByText(/无需注册/)).not.toBeInTheDocument()
    expect(screen.queryByRole('link', { name: 'AI 生成' })).not.toBeInTheDocument()
    expect(screen.queryByRole('button', { name: '绘本' })).not.toBeInTheDocument()
    expect(screen.queryByRole('button', { name: '模型管理' })).not.toBeInTheDocument()
  })

  it('点击对话框后参数行被唤醒', () => {
    renderWithTheme(<Home />)

    fireEvent.focus(screen.getByRole('textbox'))
    expect(screen.getByText('卡通风格')).toBeInTheDocument()
    expect(screen.getByText('1024 × 1024（方形·推荐）')).toBeInTheDocument()
  })

  it('已登录：对话框参数行提供「绘本」与「模型管理」入口', () => {
    useAuthStore.setState(signedIn)
    renderWithTheme(<Home />)

    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByRole('textbox')).toBeInTheDocument()

    fireEvent.focus(screen.getByRole('textbox'))
    expect(screen.getByRole('button', { name: '绘本' })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: '模型管理' })).toBeInTheDocument()

    // 旧仪表盘与顶栏导航项已被移除
    expect(screen.queryByText('绘本画家，欢迎回来')).not.toBeInTheDocument()
    expect(screen.queryByRole('link', { name: '我的绘本' })).not.toBeInTheDocument()
    expect(screen.queryByRole('button', { name: /开始创作/ })).not.toBeInTheDocument()
  })
})
