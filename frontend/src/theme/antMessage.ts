import type { MessageInstance } from 'antd/es/message/interface'

/**
 * 模块级 message 出口（docs/17 §5）。
 *
 * 非组件文件（`api/client.ts` 拦截器）拿不到 hook，只能调静态 `message.*`；
 * 而静态方法不继承主题且触发 antd 弃用警告。ThemeProvider 里的 <MessageBridge/>
 * 把 `App.useApp()` 的实例注入到这里，调用方签名与静态 message 保持一致。
 */
let instance: MessageInstance | null = null

export function bindMessage(next: MessageInstance | null): void {
  instance = next
}

export const antMessage = {
  open: (...args: Parameters<MessageInstance['open']>) => instance?.open(...args),
  success: (...args: Parameters<MessageInstance['success']>) => instance?.success(...args),
  error: (...args: Parameters<MessageInstance['error']>) => instance?.error(...args),
  info: (...args: Parameters<MessageInstance['info']>) => instance?.info(...args),
  warning: (...args: Parameters<MessageInstance['warning']>) => instance?.warning(...args),
  loading: (...args: Parameters<MessageInstance['loading']>) => instance?.loading(...args),
  destroy: (...args: Parameters<MessageInstance['destroy']>) => instance?.destroy(...args),
}
