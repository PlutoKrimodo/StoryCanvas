import { describe, expect, it } from 'vitest'

import { buildPdfFilename } from './pdf'

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
})
