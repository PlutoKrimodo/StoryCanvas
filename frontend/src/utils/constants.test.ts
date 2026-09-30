import { describe, expect, it } from 'vitest'

import {
  ART_STYLE_OPTIONS,
  IMAGE_SIZE_OPTIONS,
  MAX_PAGES_PER_BOOK,
  STORY_TEXT_MAX_LENGTH,
  TASK_STATUS_COLOR,
  TASK_STATUS_TEXT,
} from './constants'

describe('constants', () => {
  it('艺术风格选项与后端 ArtStyle 枚举数量一致', () => {
    expect(ART_STYLE_OPTIONS).toHaveLength(5)
  })

  it('任务状态文案覆盖全部五种状态', () => {
    expect(Object.keys(TASK_STATUS_TEXT)).toHaveLength(5)
    expect(TASK_STATUS_TEXT.failed).toBe('生成失败')
  })

  it('任务状态配色覆盖全部五种状态', () => {
    expect(Object.keys(TASK_STATUS_COLOR)).toHaveLength(5)
    expect(TASK_STATUS_COLOR.completed).toBe('success')
  })

  it('故事文本长度上限与后端校验一致', () => {
    expect(STORY_TEXT_MAX_LENGTH).toBe(2000)
  })

  it('尺寸选项与后端 256~2048 约束相容', () => {
    expect(IMAGE_SIZE_OPTIONS.length).toBeGreaterThan(0)
    IMAGE_SIZE_OPTIONS.forEach((option) => {
      const [width, height] = option.value.split('x').map(Number)
      expect(width).toBeGreaterThanOrEqual(256)
      expect(width).toBeLessThanOrEqual(2048)
      expect(height).toBeGreaterThanOrEqual(256)
      expect(height).toBeLessThanOrEqual(2048)
    })
  })

  it('绘本页数上限不超过后端校验值 200', () => {
    expect(MAX_PAGES_PER_BOOK).toBeGreaterThan(0)
    expect(MAX_PAGES_PER_BOOK).toBeLessThanOrEqual(200)
  })
})
