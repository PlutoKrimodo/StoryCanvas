import { DownOutlined, MenuOutlined, UserOutlined } from '@ant-design/icons'
import { Avatar, Button, Dropdown, Space } from 'antd'
import type { MenuProps } from 'antd'
import { Link, useLocation, useNavigate } from 'react-router-dom'

import { useAuth } from '../../hooks/useAuth'
import Wordmark from './Wordmark'

interface NavItem {
  to: string
  label: string
  /** 仅登录后可见 */
  authOnly?: boolean
}

const NAV_ITEMS: NavItem[] = [
  { to: '/', label: '首页' },
  { to: '/generate', label: 'AI 生成' },
  { to: '/books', label: '我的绘本', authOnly: true },
]

const isActive = (pathname: string, to: string) =>
  to === '/' ? pathname === '/' : pathname === to || pathname.startsWith(`${to}/`)

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

function AppHeader() {
  const location = useLocation()
  const navigate = useNavigate()
  const { isAuthenticated } = useAuth()

  const items = NAV_ITEMS.filter((item) => !item.authOnly || isAuthenticated)
  const mobileMenu: MenuProps = {
    items: items.map((item) => ({ key: item.to, label: item.label })),
    onClick: ({ key }) => navigate(String(key)),
  }

  return (
    <header className="sticky top-0 z-20 border-b border-hairline bg-paper/80 backdrop-blur">
      <div className="mx-auto flex h-16 w-full max-w-6xl items-center gap-6 px-4">
        <Link to="/" className="shrink-0">
          <Wordmark size="nav" />
        </Link>

        {/* 响应式显隐交给外层 div：antd 的 .ant-btn 规则优先级高于 @layer 里的 md:hidden */}
        <nav className="hidden flex-1 items-center gap-1 md:flex">
          {items.map((item) => (
            <Link
              key={item.to}
              to={item.to}
              className={`rounded-chip px-3 py-1.5 text-sm transition-colors duration-200 ${
                isActive(location.pathname, item.to)
                  ? 'bg-clay/10 font-medium text-clay'
                  : 'text-ink2 hover:text-ink'
              }`}
            >
              {item.label}
            </Link>
          ))}
        </nav>

        <div className="ml-auto flex items-center gap-2">
          <div className="md:hidden">
            <Dropdown menu={mobileMenu} trigger={['click']} placement="bottomRight">
              <Button type="text" icon={<MenuOutlined />} aria-label="导航" />
            </Dropdown>
          </div>
          <AccountActions />
        </div>
      </div>
    </header>
  )
}

export default AppHeader
