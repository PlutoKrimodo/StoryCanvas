/**
 * 前端唯一色值源（docs/17 §2.1、§2.2）。
 *
 * globals.css 的 `:root` 必须与此处同步；palette.test.ts 会双向比对，
 * 改一处漏改另一处会让 Tailwind 的 `bg-clay/20` 之类半透明用法退化成 rgb(NaN)。
 */
export const PALETTE = {
  canvas: '#FDFBF7',
  canvas2: '#F6F0E7',
  paper: '#FFFFFF',
  ink: '#2B2621',
  ink2: '#6E655C',
  ink3: '#9A9086',
  hairline: '#E9E1D6',
  hairline2: '#F1E9DD',
  clay: '#C0522F',
  clayHover: '#D2663F',
  clayActive: '#A5442A',
  amber: '#E2A13C',
  poppy: '#D2433B',
  leaf: '#6FA96B',
  sky: '#4E86BE',
  plum: '#8E5B9F',
} as const

/** `#RRGGBB` → `"r g b"`，供 `rgb(var(--x) / <alpha-value>)` 使用 */
export function hexToRgbTriplet(hex: string): string {
  const value = hex.replace('#', '')
  const code = Number.parseInt(value, 16)
  return `${(code >> 16) & 255} ${(code >> 8) & 255} ${code & 255}`
}

/** 零外链字体栈（离线演示诉求，docs/17 §2.2） */
export const FONT_SANS =
  "'PingFang SC','Hiragino Sans GB','Microsoft YaHei','Source Han Sans SC','Noto Sans CJK SC',system-ui,-apple-system,'Segoe UI',Roboto,'Helvetica Neue',sans-serif"

export const FONT_DISPLAY = "Georgia,'Songti SC','STSong','SimSun',serif"

/** globals.css `:root` 应声明的全部变量；palette.test.ts 以此为准双向比对 */
export const SC_CSS_VARS: Record<string, string> = {
  '--sc-canvas': hexToRgbTriplet(PALETTE.canvas),
  '--sc-canvas-2': hexToRgbTriplet(PALETTE.canvas2),
  '--sc-paper': hexToRgbTriplet(PALETTE.paper),
  '--sc-ink': hexToRgbTriplet(PALETTE.ink),
  '--sc-ink-2': hexToRgbTriplet(PALETTE.ink2),
  '--sc-ink-3': hexToRgbTriplet(PALETTE.ink3),
  '--sc-hairline': hexToRgbTriplet(PALETTE.hairline),
  '--sc-hairline-2': hexToRgbTriplet(PALETTE.hairline2),
  '--sc-clay': hexToRgbTriplet(PALETTE.clay),
  '--sc-clay-hover': hexToRgbTriplet(PALETTE.clayHover),
  '--sc-clay-active': hexToRgbTriplet(PALETTE.clayActive),
  '--sc-amber': hexToRgbTriplet(PALETTE.amber),
  '--sc-font-sans': FONT_SANS,
  '--sc-font-display': FONT_DISPLAY,
}

/** logo 五色环：仅用于逐字母上色、选中态与状态点，不用于小字 */
export const LOGO_COLORS = [PALETTE.poppy, PALETTE.amber, PALETTE.leaf, PALETTE.sky, PALETTE.plum]
