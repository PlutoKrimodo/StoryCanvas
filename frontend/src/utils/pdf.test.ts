import { describe, expect, it } from 'vitest'

import { buildCoverSubtitle, buildPageLabel, buildPdfFilename } from './pdf'

describe('pdf 工具', () => {
  it('默认文件名为 storycanvas.pdf', () => {
    expect(buildPdfFilename('')).toBe('storycanvas.pdf')
  })

  it('使用绘本标题作为文件名并追加 .pdf', () => {
    expect(buildPdfFilename('小红帽的故事')).toBe('小红帽的故事.pdf')
  })

  it('过滤文件名非法字符', () => {
    expect(buildPdfFilename('a/b:c*d?e"f<g>h|i')).toBe('a_b_c_d_e_f_g_h_i.pdf')
  })

  it('去掉首尾空白后为空时回退默认名', () => {
    expect(buildPdfFilename('   ')).toBe('storycanvas.pdf')
  })

  it('生成连续页码', () => {
    expect(buildPageLabel(1, 3)).toBe('第 1 / 3 页')
    expect(buildPageLabel(2, 3)).toBe('第 2 / 3 页')
    expect(buildPageLabel(3, 3)).toBe('第 3 / 3 页')
  })

  it('页码越界时收敛到有效范围', () => {
    expect(buildPageLabel(0, 3)).toBe('第 1 / 3 页')
    expect(buildPageLabel(4, 3)).toBe('第 3 / 3 页')
  })

  it('封面副标题显示总页数', () => {
    expect(buildCoverSubtitle(3)).toBe('StoryCanvas · 共 3 页')
    expect(buildCoverSubtitle(0)).toBe('StoryCanvas · 共 0 页')
  })
})
