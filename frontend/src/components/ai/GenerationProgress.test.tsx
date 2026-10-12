import { act, cleanup, fireEvent, render, screen } from '@testing-library/react'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import GenerationProgress from './GenerationProgress'
import { TaskStatus, type TaskResult } from '../../types/generation'

const START = Date.parse('2026-10-12T00:00:00.000Z')

function makeTask(over: Partial<TaskResult> = {}): TaskResult {
  return {
    task_id: 't1',
    status: TaskStatus.PROCESSING,
    prompt: '',
    created_at: new Date(START).toISOString(),
    ...over,
  }
}

/** antd 进度条的百分比：100% 成功态会把文字换成图标，故优先读百分比文字、退回条体宽度 */
function percentOf(container: HTMLElement): number {
  const text = container.querySelector('.ant-progress-text')?.textContent ?? ''
  const matched = text.match(/(\d+)/)
  if (matched) {
    return Number(matched[1])
  }
  const bg = container.querySelector<HTMLElement>('.ant-progress-bg')
  return bg ? Number.parseFloat(bg.style.width) : NaN
}

describe('GenerationProgress 时间缓动（docs/17 §4.4）', () => {
  beforeEach(() => {
    vi.useFakeTimers()
    // 任务已跑 90s：缓动值确定可复算
    vi.setSystemTime(START + 90_000)
  })

  afterEach(() => {
    cleanup()
    vi.useRealTimers()
  })

  it('生成中：进度随时间持续爬升，但活跃期不收束到 100', () => {
    const onCancel = vi.fn()
    const { container } = render(<GenerationProgress task={makeTask()} polling onCancel={onCancel} />)

    const before = percentOf(container)
    // 旧的假阶梯把 processing 永远钉在 60%
    expect(before).toBeGreaterThan(60)
    expect(before).toBeLessThan(92)
    expect(screen.getByText('已用 90s · 通常 20–40s')).toBeInTheDocument()

    act(() => {
      vi.advanceTimersByTime(30_000)
    })
    expect(percentOf(container)).toBeGreaterThan(before)
    expect(screen.getByText('已用 120s · 通常 20–40s')).toBeInTheDocument()

    fireEvent.click(screen.getByRole('button', { name: '取消生成' }))
    expect(onCancel).toHaveBeenCalledTimes(1)
  })

  it('阶段条按状态推进：解析故事 → 优化提示词 → 绘制插画 → 完成', () => {
    render(<GenerationProgress task={makeTask({ status: TaskStatus.PENDING })} polling onCancel={vi.fn()} />)
    expect(screen.getByText('等待中')).toBeInTheDocument()
    ;['解析故事', '优化提示词', '绘制插画', '完成'].forEach((title) => {
      expect(screen.getByText(title)).toBeInTheDocument()
    })
  })

  it('轮询已停（5 分钟兜底）时说明状态同步停了，而不是继续假装在跑', () => {
    render(<GenerationProgress task={makeTask()} polling={false} onCancel={vi.fn()} />)
    expect(screen.getByText('状态同步已暂停，可稍后重新开始')).toBeInTheDocument()
  })

  it('完成：收束 100%、无计时行、可重新生成；失败：透出错误原因', () => {
    const onRegenerate = vi.fn()
    const { container, unmount } = render(
      <GenerationProgress
        task={makeTask({ status: TaskStatus.COMPLETED })}
        polling={false}
        onCancel={vi.fn()}
        onRegenerate={onRegenerate}
      />,
    )
    expect(percentOf(container)).toBe(100)
    expect(screen.queryByText(/已用/)).not.toBeInTheDocument()
    expect(screen.queryByRole('button', { name: '取消生成' })).not.toBeInTheDocument()
    fireEvent.click(screen.getByRole('button', { name: '重新生成' }))
    expect(onRegenerate).toHaveBeenCalledTimes(1)

    unmount()
    render(
      <GenerationProgress
        task={makeTask({ status: TaskStatus.FAILED, error_message: '出图服务返回 400 InvalidParameter' })}
        polling={false}
        onCancel={vi.fn()}
      />,
    )
    expect(screen.getByText('出图服务返回 400 InvalidParameter')).toBeInTheDocument()
  })

  it('提示词默认两行截断，展开后看全文', () => {
    render(
      <GenerationProgress
        task={makeTask({ prompt: '一幅水彩绘本插画：小兔子提着灯笼穿过森林' })}
        polling
        onCancel={vi.fn()}
      />,
    )
    expect(screen.getByText(/绘图提示词：/).className).toContain('line-clamp-2')
    fireEvent.click(screen.getByText('查看提示词'))
    expect(screen.getAllByText(/小兔子提着灯笼/).length).toBeGreaterThan(1)
  })
})
