import type { ThemeConfig } from 'antd'

import { FONT_SANS, PALETTE } from './palette'

/**
 * AntD 5 主题（docs/17 §2.4）。键位按 antd 5.29.3 的类型定义逐条核对，
 * 特别注意 Button 没有 `borderRadius` component token（写了会 TS 报错）。
 */
export const antdTheme: ThemeConfig = {
  cssVar: true,
  token: {
    colorPrimary: PALETTE.clay,
    colorLink: PALETTE.clay,
    colorWarning: PALETTE.amber,
    colorSuccess: PALETTE.leaf,

    colorText: PALETTE.ink,
    colorTextSecondary: PALETTE.ink2,
    colorTextTertiary: PALETTE.ink3,

    colorBorder: PALETTE.hairline,
    colorBorderSecondary: PALETTE.hairline2,

    colorBgLayout: PALETTE.canvas,
    colorBgContainer: PALETTE.paper,
    colorBgElevated: PALETTE.paper,
    colorFillQuaternary: PALETTE.canvas2,

    borderRadius: 12,
    // genRadius 在 base<16 时会把 LG 封顶到 base+2，必须显式覆盖，
    // 生效后页面里 9 处 `!rounded-2xl` 才有统一替代
    borderRadiusLG: 16,
    borderRadiusSM: 8,

    fontFamily: FONT_SANS,
    fontSize: 14,

    boxShadow: '0 1px 2px rgba(43,38,33,.04), 0 8px 24px -14px rgba(43,38,33,.10)',
    boxShadowSecondary: '0 18px 40px -18px rgba(43,38,33,.22)',
  },
  components: {
    Card: {
      headerBg: PALETTE.paper,
      bodyPadding: 24,
      extraColor: PALETTE.ink2,
      actionsBg: PALETTE.canvas,
    },
    Button: {
      fontWeight: 500,
      primaryShadow: 'none',
      defaultShadow: 'none',
      iconGap: 8,
    },
    Tag: {
      defaultBg: PALETTE.canvas2,
      defaultColor: PALETTE.ink2,
    },
    Progress: {
      defaultColor: PALETTE.clay,
      remainingColor: PALETTE.hairline2,
      lineBorderRadius: 999,
    },
    Collapse: {
      headerBg: 'transparent',
      contentBg: 'transparent',
    },
    Skeleton: {
      color: PALETTE.canvas2,
      gradientFromColor: PALETTE.canvas,
      gradientToColor: '#EFE7DA',
      blockRadius: 14,
    },
    Tabs: {
      inkBarColor: PALETTE.clay,
      itemColor: PALETTE.ink3,
      itemActiveColor: PALETTE.ink,
    },
  },
}
