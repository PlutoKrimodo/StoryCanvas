import { act, cleanup, fireEvent, render, screen } from '@testing-library/react'
import { afterEach, describe, expect, it, vi } from 'vitest'

import ResultPreview from './ResultPreview'
import { imagesApi } from '../../api/images'
import { TaskStatus, type ParsedData, type TaskResult } from '../../types/generation'

vi.mock('../../api/images', () => ({
  imagesApi: { download: vi.fn().mockResolvedValue(undefined) },
}))

const parsed: ParsedData = {
  characters: [{ name: '小兔子' }],
  scene: { location: '森林', mood: '温暖' },
  objects: ['灯笼'],
  actions: ['穿过小溪'],
  emotions: ['好奇'],
  style: {},
}

function makeTask(over: Partial<TaskResult> = {}): TaskResult {
  return {
    task_id: 't1',
    status: TaskStatus.COMPLETED,
    prompt: '一幅水彩绘本插画：小兔子提着灯笼穿过森林',
    original_text: '小兔子第一次独自走过森林。',
    parsed_data: parsed,
    result: { image_id: 'img_1', image_url: 'https://cdn.example.com/img_1.png' },
    created_at: '2026-10-12T00:00:00.000Z',
    ...over,
  }
}

describe('ResultPreview 降噪与操作重排（docs/17 §4.5）', () => {
  afterEach(() => {
    cleanup()
    vi.clearAllMocks()
  })

  it('解析结果档案化：五组圆点文本，不再是一堆 Tag', () => {
    const { container } = render(
      <ResultPreview task={makeTask()} onRegenerate={vi.fn()} onSaveToBook={vi.fn()} />,
    )
    fireEvent.click(screen.getByText('查看提示词与解析结果'))

    expect(screen.getByText('小兔子')).toBeInTheDocument()
    expect(screen.getByText('地点：森林')).toBeInTheDocument()
    expect(screen.getByText('氛围：温暖')).toBeInTheDocument()
    expect(container.querySelectorAll('.ant-tag')).toHaveLength(0)
    // 每个值一个 3px 圆点：1 + 2 + 1 + 1 + 1
    expect(container.querySelectorAll('.h-\\[3px\\].w-\\[3px\\]')).toHaveLength(6)
  })

  it('操作条：保存到绘本是唯一 primary，下载与重新生成各自生效', async () => {
    const onSaveToBook = vi.fn()
    const onRegenerate = vi.fn()
    render(<ResultPreview task={makeTask()} onRegenerate={onRegenerate} onSaveToBook={onSaveToBook} />)

    // anticon 的 aria-label 会进按钮可访问名，中文双字还会被插全角空格 → 一律用正则
    const save = screen.getByRole('button', { name: /保存到绘本/ })
    const download = screen.getByRole('button', { name: /下\s*载/ })
    expect(save.className).toContain('ant-btn-primary')
    fireEvent.click(save)
    expect(onSaveToBook).toHaveBeenCalledWith('img_1')

    fireEvent.click(download)
    expect(imagesApi.download).toHaveBeenCalledWith('img_1')
    // 在 act 内冲刷下载 Promise，避免 setDownloading(false) 落在 act 之外
    await act(async () => {
      await Promise.resolve()
    })

    fireEvent.click(screen.getByRole('button', { name: /重新生成/ }))
    expect(onRegenerate).toHaveBeenCalledTimes(1)
  })

  it('原文为引用块，详情默认收起', () => {
    render(<ResultPreview task={makeTask()} onRegenerate={vi.fn()} onSaveToBook={vi.fn()} />)
    expect(screen.getByText('小兔子第一次独自走过森林。').closest('.border-l-2')).not.toBeNull()
    expect(screen.queryByText(/小兔子提着灯笼/)).not.toBeInTheDocument()

    fireEvent.click(screen.getByText('查看提示词与解析结果'))
    expect(screen.getByText(/提示词：一幅水彩绘本插画/)).toBeInTheDocument()
  })

  it('无图兜底：Empty + 保存/下载禁用', () => {
    render(
      <ResultPreview
        task={makeTask({ result: null })}
        onRegenerate={vi.fn()}
        onSaveToBook={vi.fn()}
      />,
    )
    expect(screen.getByText('暂无生成图')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /保存到绘本/ })).toBeDisabled()
    expect(screen.getByRole('button', { name: /下\s*载/ })).toBeDisabled()
  })
})
