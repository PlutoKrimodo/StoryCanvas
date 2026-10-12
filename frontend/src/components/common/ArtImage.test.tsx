import { fireEvent, render, screen } from '@testing-library/react'
import { describe, expect, it } from 'vitest'

import ArtImage from './ArtImage'

describe('ArtImage', () => {
  it('加载中出骨架，成功后图片挂 .sc-artimg 淡入', () => {
    const { container } = render(<ArtImage src="https://example.com/a.png" alt="生成结果" />)
    const img = screen.getByRole('img', { name: '生成结果' })

    expect(container.querySelector('.sc-skeleton')).toBeInTheDocument()
    expect(img.className).toContain('invisible')

    fireEvent.load(img)
    expect(container.querySelector('.sc-skeleton')).not.toBeInTheDocument()
    expect(img.className).toContain('sc-artimg')
  })

  it('失败出占位与重试，重试回到加载态', () => {
    const { container } = render(<ArtImage src="https://example.com/b.png" alt="生成结果" />)
    const img = screen.getByRole('img', { name: '生成结果' })

    fireEvent.error(img)
    expect(screen.getByText('图片加载失败')).toBeInTheDocument()
    expect(container.querySelector('.sc-skeleton')).not.toBeInTheDocument()

    fireEvent.click(screen.getByRole('button', { name: /重试/ }))
    expect(screen.queryByText('图片加载失败')).not.toBeInTheDocument()
  })

  it('maxHeight 同时约束骨架与图片', () => {
    render(<ArtImage src="https://example.com/c.png" alt="生成结果" maxHeight={420} />)
    expect(screen.getByRole('img', { name: '生成结果' })).toHaveStyle({ maxHeight: '420px' })
  })
})
