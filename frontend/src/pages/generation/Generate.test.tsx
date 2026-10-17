import { cleanup, fireEvent, screen } from '@testing-library/react'
import { afterEach, describe, expect, it, vi } from 'vitest'

import Generate from './Generate'
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

describe('对话式生成页', () => {
  afterEach(() => {
    cleanup()
    useAuthStore.setState({ isAuthenticated: false, user: null })
  })

  it('游客：展示创作引导、常驻参数与单张限制', () => {
    renderWithTheme(<Generate />, { at: '/generate' })

    expect(screen.getByRole('heading', { name: '创作对话' })).toBeInTheDocument()
    expect(screen.getByText(/把一个故事，聊成一本连环画/)).toBeInTheDocument()
    expect(screen.getByText(/游客可免费生成 1 张/)).toBeInTheDocument()
    expect(screen.getByText('卡通风格')).toBeInTheDocument()
    expect(screen.getByText('1024 × 1024（方形·推荐）')).toBeInTheDocument()
    expect(screen.getByText('1 张')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: '发送' })).toBeDisabled()
  })

  it('点击示例提示会回填输入框', () => {
    renderWithTheme(<Generate />, { at: '/generate' })

    fireEvent.click(screen.getByText(/小女孩在雨后的森林里/))
    expect(screen.getByRole('textbox')).toHaveValue(
      '小女孩在雨后的森林里发现一只受伤的小鸟，她把它捧在手心带回家。',
    )
  })
})
