import { fireEvent, render, screen } from '@testing-library/react'
import { MemoryRouter, useLocation } from 'react-router-dom'
import { afterEach, describe, expect, it } from 'vitest'

import { useAuthStore } from '../../stores/authStore'
import Home from './Home'

// ResizeObserver / matchMedia 的 jsdom 兜底见 src/test/setup.ts

function LocationProbe() {
  const location = useLocation()
  return <div data-testid="path">{location.pathname}</div>
}

const renderHome = () =>
  render(
    <MemoryRouter initialEntries={['/']}>
      <Home />
      <LocationProbe />
    </MemoryRouter>,
  )

describe('Home 双形态', () => {
  afterEach(() => {
    useAuthStore.setState({ isAuthenticated: false, user: null })
  })

  it('游客：hero 居中 + 生成器可直接使用 + 额度小字', () => {
    renderHome()
    expect(screen.getByRole('img', { name: 'StoryCanvas' })).toBeInTheDocument()
    expect(screen.getByText('把一段故事，画成一幅绘本插画')).toBeInTheDocument()
    expect(screen.getByRole('textbox')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /立即生成/ })).toBeInTheDocument()
    expect(screen.getByText(/无需注册/)).toBeInTheDocument()
    // hero 形态不出顶栏导航
    expect(screen.queryByRole('link', { name: 'AI 生成' })).not.toBeInTheDocument()
  })

  it('已登录：4 条入口跳转（CTA + 三个 tile）全部可达', () => {
    useAuthStore.setState({
      isAuthenticated: true,
      user: { id: 'u1', username: '绘本画家', email: 'painter@example.com' },
    })
    renderHome()
    expect(screen.getByText('绘本画家，欢迎回来')).toBeInTheDocument()

    const targets = [
      [/开始创作/, '/generate'],
      [/我的绘本/, '/books'],
      [/个人中心/, '/profile'],
      [/导出 PDF/, '/books'],
    ] as const

    for (const [name, expected] of targets) {
      fireEvent.click(screen.getByRole('button', { name }))
      expect(screen.getByTestId('path')).toHaveTextContent(expected)
    }
  })
})
