import { describe, expect, it } from 'vitest'

import globalsCss from '../styles/globals.css?raw'
import { PALETTE, SC_CSS_VARS, hexToRgbTriplet } from './palette'

/**
 * 令牌只允许一处定义、另一处镜像：CSS 侧写在 globals.css 的 `:root`，
 * JS 侧写在 palette.ts。漏改任一侧会让 `bg-clay/20` 这类半透明用法变成 rgb(NaN)，
 * 而这类错误在页面上只是「颜色变淡」，很难被发现（docs/17 §8.1）。
 */
const rootBlock = /:root\s*\{([^}]*)\}/.exec(globalsCss)?.[1] ?? ''
const squash = (value: string) => value.replace(/\s+/g, '')

const declared = new Map<string, string>()
for (const line of rootBlock.split('\n')) {
  const entry = /^\s*(--sc-[a-z0-9-]+)\s*:\s*([^;]+);/.exec(line)
  if (entry) declared.set(entry[1], squash(entry[2]))
}

const luminance = (hex: string) => {
  const code = Number.parseInt(hex.replace('#', ''), 16)
  const channel = (value: number) => {
    const srgb = value / 255
    return srgb <= 0.03928 ? srgb / 12.92 : ((srgb + 0.055) / 1.055) ** 2.4
  }
  return (
    0.2126 * channel((code >> 16) & 255) + 0.7152 * channel((code >> 8) & 255) + 0.0722 * channel(code & 255)
  )
}

const contrast = (a: string, b: string) => {
  const [lighter, darker] = [luminance(a), luminance(b)].sort((x, y) => y - x)
  return (lighter + 0.05) / (darker + 0.05)
}

describe('设计令牌双源同步', () => {
  it('palette.ts 的每个变量都在 globals.css :root 中同值声明', () => {
    for (const [name, value] of Object.entries(SC_CSS_VARS)) {
      expect(declared.has(name), `${name} 未在 globals.css 声明`).toBe(true)
      expect(squash(value), `${name} 两侧取值不一致`).toBe(declared.get(name))
    }
  })

  it('globals.css 无 SC_CSS_VARS 之外的孤儿 --sc-* 变量', () => {
    expect([...declared.keys()].filter((name) => !(name in SC_CSS_VARS))).toEqual([])
  })

  it('hex → rgb 三元组换算正确', () => {
    expect(hexToRgbTriplet(PALETTE.clay)).toBe('192 82 47')
    expect(hexToRgbTriplet(PALETTE.canvas)).toBe('253 251 247')
    for (const hex of Object.values(PALETTE)) {
      expect(hex).toMatch(/^#[0-9A-F]{6}$/)
    }
  })

  it('陶土主色在白底上满足 WCAG AA(4.5)', () => {
    expect(contrast(PALETTE.clay, PALETTE.paper)).toBeGreaterThanOrEqual(4.5)
  })

  it('样式表零外链资源（离线演示诉求）', () => {
    expect(globalsCss).not.toMatch(/@import|url\(['"]?https?:\/\//)
  })
})
