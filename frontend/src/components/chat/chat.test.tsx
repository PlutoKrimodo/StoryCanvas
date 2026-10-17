import { cleanup, fireEvent, screen } from '@testing-library/react'
import { afterEach, describe, expect, it, vi } from 'vitest'

import StoryboardMessage from './StoryboardMessage'
import { renderWithTheme } from '../../test/renderWithTheme'
import type { ChatMessage } from '../../types/conversation'

const message: ChatMessage = {
  id: 'm1',
  role: 'assistant',
  messageType: 'storyboard',
  content: '雨后的小鸟',
  createdAt: '2026-10-18T00:00:00Z',
  payload: {
    batch_id: 'b1',
    status: 'draft',
    panels: [
      { index: 0, text: '森林里发现小鸟' },
      { index: 1, text: '把小鸟捧在手心' },
    ],
    consistency: {
      characters: [{ name: '小女孩', appearance: '红色雨衣' }],
      palette: '暖黄与苔绿',
    },
  },
}

describe('StoryboardMessage', () => {
  afterEach(cleanup)

  it('逐格编辑后按编辑结果开始绘制', () => {
    const onStart = vi.fn()
    renderWithTheme(
      <StoryboardMessage message={message} onStart={onStart} originalText="故事" />,
    )

    const first = screen.getByLabelText('第 1 格描述')
    fireEvent.change(first, { target: { value: '改过的第一格' } })
    fireEvent.click(screen.getByRole('button', { name: /开始画这 2 张/ }))

    expect(onStart).toHaveBeenCalledTimes(1)
    expect(onStart.mock.calls[0][0][0].text).toBe('改过的第一格')
  })

  it('提交后不再显示编辑按钮', () => {
    renderWithTheme(
      <StoryboardMessage
        message={{ ...message, payload: { ...message.payload, status: 'pending' } }}
        onStart={vi.fn()}
      />,
    )

    expect(screen.queryByRole('button', { name: /开始画这/ })).not.toBeInTheDocument()
    expect(screen.getByText('分镜已提交，正在生成图片。')).toBeInTheDocument()
  })
})
