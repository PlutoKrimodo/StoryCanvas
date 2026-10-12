import { fireEvent, render, screen } from '@testing-library/react'
import { describe, expect, it, vi } from 'vitest'

import { ArtStyle } from '../../types/generation'
import StyleSelector from './StyleSelector'

const LABELS = ['卡通风格', '水彩风格', '油画风格', '动漫风格', '铅笔素描']

describe('StyleSelector 色卡', () => {
  it('渲染 5 张卡，仅选中项 aria-pressed=true', () => {
    render(<StyleSelector value={ArtStyle.OIL_PAINTING} onChange={() => {}} />)
    const cards = screen.getAllByRole('button')

    expect(cards).toHaveLength(5)
    LABELS.forEach((label) => {
      expect(screen.getByRole('button', { name: new RegExp(label) })).toBeInTheDocument()
    })
    expect(cards[2]).toHaveAttribute('aria-pressed', 'true')
    ;[0, 1, 3, 4].forEach((index) => expect(cards[index]).toHaveAttribute('aria-pressed', 'false'))
  })

  it('点击回传对应枚举，disabled 后不再触发', () => {
    const onChange = vi.fn()
    const { rerender } = render(<StyleSelector onChange={onChange} />)
    fireEvent.click(screen.getByRole('button', { name: /动漫风格/ }))
    expect(onChange).toHaveBeenLastCalledWith(ArtStyle.ANIME)

    rerender(
      <StyleSelector onChange={onChange} disabled />,
    )
    fireEvent.click(screen.getByRole('button', { name: /动漫风格/ }))
    expect(onChange).toHaveBeenCalledTimes(1)
  })

  it('未选值也能渲染（受控 value 可为 undefined）', () => {
    render(<StyleSelector />)
    screen.getAllByRole('button').forEach((card) => expect(card).toHaveAttribute('aria-pressed', 'false'))
  })
})
