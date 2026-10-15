import { cleanup, fireEvent, screen } from '@testing-library/react'
import { Route, Routes } from 'react-router-dom'
import { afterEach, describe, expect, it, vi } from 'vitest'

import Generate from './Generate'
import { renderWithTheme } from '../../test/renderWithTheme'
import { useAuthStore } from '../../stores/authStore'
import { useGenerationStore } from '../../stores/generationStore'
import { TaskStatus } from '../../types/generation'
import { ART_STYLE_OPTIONS } from '../../utils/constants'

vi.mock('../../api/generation', () => ({
  generationApi: {
    getHistory: () => Promise.resolve({ data: { data: { items: [], total: 0 } } }),
  },
}))

describe('生成页接入新体系（docs/17 Step 9 冒烟）', () => {
  afterEach(() => {
    cleanup()
    useGenerationStore.setState({ currentTask: null, polling: false })
  })

  it('无任务时：PageHeader + 表单卡 + 引导占位 + 历史卡一次性挂载', async () => {
    const { container } = renderWithTheme(<Generate />, { at: '/generate' })

    expect(screen.getByRole('heading', { name: 'AI 绘本插画生成' })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /生成插画/ })).toBeInTheDocument()
    expect(screen.getByText('生成历史')).toBeInTheDocument()
    ART_STYLE_OPTIONS.forEach((option) => {
      expect(
        screen.getByRole('button', { name: new RegExp(option.label) }),
      ).toBeInTheDocument()
    })

    // 历史为空 → 占位文案；同时说明 store 的异步 fetch 能在 act 内收敛
    expect(await screen.findByText('还没有生成记录')).toBeInTheDocument()
    expect(screen.getByText(/生成结果会显示在这里/)).toBeInTheDocument()

    // 底色只由 AppLayout 单点提供，页面自身不再叠一层满屏容器
    expect(container.querySelectorAll('.min-h-screen')).toHaveLength(1)
    expect(container.querySelector('.bg-gradient-to-br')).toBeNull()
  })

  it('游客点击「保存到绘本」：引导登录而非死路弹窗（Q3②）', () => {
    useAuthStore.setState({
      isAuthenticated: false,
      user: null,
      accessToken: null,
      refreshToken: null,
    })
    useGenerationStore.setState({
      currentTask: {
        task_id: 't9',
        status: TaskStatus.COMPLETED,
        prompt: '',
        created_at: new Date().toISOString(),
        result: { image_id: 'img-1', image_url: '/api/v1/images/img-1/raw' },
      },
      polling: false,
    })

    renderWithTheme(
      <Routes>
        <Route path="/generate" element={<Generate />} />
        <Route path="/login" element={<div>登录页占位</div>} />
      </Routes>,
      { at: '/generate' },
    )

    fireEvent.click(screen.getByRole('button', { name: /保存到绘本/ }))

    expect(screen.getByText('登录页占位')).toBeInTheDocument()
  })
})
