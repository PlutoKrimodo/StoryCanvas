import type { ReactNode } from 'react'
import { Link } from 'react-router-dom'

import { AccountActions } from './AppHeader'
import AppHeader from './AppHeader'
import Wordmark from './Wordmark'

type Variant = 'hero' | 'app' | 'auth'

interface AppLayoutProps {
  /** hero=游客首页；app=带顶栏的功能页；auth=登录/注册 */
  variant?: Variant
  children: ReactNode
  className?: string
}

/** 全站唯一背景与导航来源（docs/17 §3.1）：页面不再自带 min-h-screen 渐变容器 */
function AppLayout({ variant = 'app', children, className = '' }: AppLayoutProps) {
  if (variant === 'hero') {
    return (
      <div className={`sc-canvas-wash relative min-h-screen bg-canvas ${className}`}>
        <div className="absolute right-4 top-4 z-20 sm:right-6 sm:top-5">
          <AccountActions ghost />
        </div>
        <main className="mx-auto w-full max-w-2xl px-4 pb-16 pt-[12vh]">{children}</main>
      </div>
    )
  }

  if (variant === 'auth') {
    return (
      <div
        className={`flex min-h-screen flex-col items-center justify-center gap-8 bg-canvas px-4 py-10 ${className}`}
      >
        <Wordmark size="nav" />
        <div className="animate-enter w-full max-w-md">{children}</div>
        <Link to="/" className="text-sm text-ink2 transition-colors duration-200 hover:text-clay">
          返回体验首页
        </Link>
      </div>
    )
  }

  return (
    <div className={`flex min-h-screen flex-col bg-canvas ${className}`}>
      <AppHeader />
      <main className="mx-auto w-full max-w-6xl flex-1 px-4 pb-12 pt-8">{children}</main>
    </div>
  )
}

export default AppLayout
