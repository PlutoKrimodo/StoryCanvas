import { describe, expect, it } from 'vitest'

import { ART_STYLE_OPTIONS, STORY_TEXT_MAX_LENGTH, TASK_STATUS_TEXT } from './constants'

describe('constants', () => {
  it('艺术风格选项与后端 ArtStyle 枚举数量一致', () => {
    expect(ART_STYLE_OPTIONS).toHaveLength(5)
  })

  it('任务状态文案覆盖全部五种状态', () => {
    expect(Object.keys(TASK_STATUS_TEXT)).toHaveLength(5)
    expect(TASK_STATUS_TEXT.failed).toBe('生成失败')
  })

  it('故事文本长度上限与后端校验一致', () => {
    expect(STORY_TEXT_MAX_LENGTH).toBe(2000)
  })
})
