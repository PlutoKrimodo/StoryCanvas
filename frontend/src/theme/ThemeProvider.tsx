import { App as AntdApp, ConfigProvider } from 'antd'
import zhCN from 'antd/locale/zh_CN'
import { useEffect, type ReactNode } from 'react'

import { bindMessage } from './antMessage'
import { antdTheme } from './antdTheme'

/** 在 <AntdApp> 内部把 useApp() 的 message 实例交给 antMessage，供非组件文件调用 */
function MessageBridge() {
  const { message } = AntdApp.useApp()

  useEffect(() => {
    bindMessage(message)
    return () => bindMessage(null)
  }, [message])

  return null
}

/**
 * 主题底座（docs/17 §2.4）：色板 + 圆角 + 中文 locale + message 桥。
 * `component={false}` 不额外插 DOM，页面容器与路由结构保持原样。
 */
export default function ThemeProvider({ children }: { children: ReactNode }) {
  return (
    <ConfigProvider theme={antdTheme} locale={zhCN}>
      <AntdApp component={false}>
        <MessageBridge />
        {children}
      </AntdApp>
    </ConfigProvider>
  )
}
