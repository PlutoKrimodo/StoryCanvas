import { cleanup, screen } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'

import GuestGenerator from './GuestGenerator'
import { renderWithTheme } from '../../test/renderWithTheme'
import { useGenerationStore } from '../../stores/generationStore'
import { TaskStatus, type TaskResult } from '../../types/generation'

function seedFailedTask(over: Partial<TaskResult> = {}) {
  useGenerationStore.setState({
    currentTask: {
      task_id: 't1',
      status: TaskStatus.FAILED,
      prompt: '',
      created_at: new Date().toISOString(),
      ...over,
    },
    polling: false,
  })
}

/**
 * Q8：游客生成失败时，具体原因（如「生成图片超过大小上限…」）由
 * GenerationProgress 的错误条展示；GuestGenerator 只补下一步引导，
 * 且不得再臆断为「额度用完」或重复展示同一条消息。
 */
describe('GuestGenerator 失败提示（Q8）', () => {
  afterEach(() => {
    cleanup()
    useGenerationStore.setState({ currentTask: null, polling: false })
  })

  it('展示后端返回的具体失败原因（且不重复）', () => {
    seedFailedTask({
      error_message: '生成图片超过大小上限（11.2MB > 10MB），已中止保存；请重试或调整参数',
    })

    renderWithTheme(<GuestGenerator />)

    // 恰好一处：错误条来自 GenerationProgress，引导条不重复该消息
    expect(screen.getAllByText(/大小上限/)).toHaveLength(1)
    expect(screen.queryByText(/游客额度可能已用完/)).not.toBeInTheDocument()
  })

  it('无 error_message 时展示通用引导文案', () => {
    seedFailedTask()

    renderWithTheme(<GuestGenerator />)

    expect(screen.getByText(/可稍后重试或调整参数/)).toBeInTheDocument()
  })
})
