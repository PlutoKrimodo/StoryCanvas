import { useState } from 'react'
import { DownOutlined, UserOutlined } from '@ant-design/icons'
import { Avatar, Button, Dropdown, Space } from 'antd'
import type { MenuProps } from 'antd'
import { Link, useNavigate } from 'react-router-dom'

import { useAuth } from '../../hooks/useAuth'
import Wordmark from './Wordmark'

/** 登录/游客身份入口：顶栏右侧与 hero 右上角共用 */
export function AccountActions({ ghost = false }: { ghost?: boolean }) {
  const navigate = useNavigate()
  const { user, isAuthenticated, signOut } = useAuth()

  if (!isAuthenticated) {
    return (
      <Space size={8}>
        <Button type={ghost ? 'text' : 'default'} onClick={() => navigate('/login')}>
          登录
        </Button>
        <Button type="primary" shape="round" onClick={() => navigate('/register')}>
          注册
        </Button>
      </Space>
    )
  }

  const menu: MenuProps = {
    items: [
      { key: 'profile', label: '个人中心' },
      { type: 'divider' },
      { key: 'logout', label: '退出登录', danger: true },
    ],
    onClick: ({ key }) => {
      if (key === 'profile') navigate('/profile')
      else void signOut()
    },
  }

  return (
    <Dropdown menu={menu} trigger={['click']} placement="bottomRight">
      <Button type="text">
        <Space size={6}>
          <Avatar size={28} src={user?.avatar || undefined} icon={<UserOutlined />} />
          <span className="max-w-[10rem] truncate">{user?.username}</span>
          <DownOutlined style={{ fontSize: 10 }} />
        </Space>
      </Button>
    </Dropdown>
  )
}

interface AppHeaderProps {
  /** 悬浮模式：静止完全透明，鼠标移入整条导航后变为完全不透明 */
  transparent?: boolean
  /** 是否显示左上角 LOGO（首页态大 LOGO 在页面中央，顶栏不显示） */
  showLogo?: boolean
}

/**
 * 全站顶栏。
 *
 * 统一创作页使用悬浮模式：静止时完全透明（仅保留登录/注册按钮），
 * 鼠标移入后整条导航变为不透明，移出恢复透明。
 * 首页与创作已合并，导航项只保留账户入口，不再有页面跳转项。
 */
function AppHeader({ transparent = false, showLogo = true }: AppHeaderProps) {
  const [hovered, setHovered] = useState(false)

  const headerClass = transparent
    ? hovered
      ? 'border-b border-hairline bg-paper shadow-sm'
      : 'border-b border-transparent bg-transparent shadow-none'
    : 'border-b border-hairline bg-paper/80 backdrop-blur'

  return (
    <header
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      className={`sticky top-0 z-30 transition-[background-color,border-color,box-shadow] duration-300 ${headerClass}`}
    >
      <div className="mx-auto flex h-16 w-full max-w-6xl items-center gap-6 px-4">
        {showLogo ? (
          <Link to="/" className="shrink-0 animate-fade">
            <Wordmark size="nav" />
          </Link>
        ) : null}

        <div className="ml-auto flex items-center gap-2">
          <AccountActions />
        </div>
      </div>
    </header>
  )
}

export default AppHeader
