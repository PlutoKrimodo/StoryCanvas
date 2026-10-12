import { render } from '@testing-library/react'
import { MemoryRouter } from 'react-router-dom'
import type { ReactNode } from 'react'

import ThemeProvider from '../theme/ThemeProvider'

interface Options {
  /** 初始路由 */
  at?: string
}

/**
 * 与 main.tsx 同构的测试外壳：ThemeProvider 提供主题、zh_CN locale 与
 * `<App>`（`App.useApp()` 的 message 上下文）；缺了它页面级组件会拿不到 message。
 */
export function renderWithTheme(ui: ReactNode, { at = '/' }: Options = {}) {
  return render(
    <MemoryRouter initialEntries={[at]}>
      <ThemeProvider>{ui}</ThemeProvider>
    </MemoryRouter>,
  )
}
