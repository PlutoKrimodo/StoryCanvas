import {
  BookOutlined,
  FilePdfOutlined,
  ThunderboltOutlined,
  UserOutlined,
} from '@ant-design/icons'
import { Button, Typography } from 'antd'
import type { ReactNode } from 'react'
import { useNavigate } from 'react-router-dom'

import GuestGenerator from '../../components/ai/GuestGenerator'
import AppLayout from '../../components/layout/AppLayout'
import PageHeader from '../../components/layout/PageHeader'
import Wordmark from '../../components/layout/Wordmark'
import { useAuth } from '../../hooks/useAuth'
import { useAuthStore } from '../../stores/authStore'
import { GUEST_GENERATION_LIMIT } from '../../utils/constants'

const { Paragraph, Text } = Typography

interface Entry {
  key: string
  title: string
  description: string
  to: string
  icon: ReactNode
  accent: string
}

function Home() {
  const navigate = useNavigate()
  const { user, isAuthenticated } = useAuth()
  const guestQuota = useAuthStore((state) => state.guestQuota)

  // Guest-first：游客看到的是 Google 式居中 hero（logo + 生成器 + 额度小字），无导航干扰
  if (!isAuthenticated) {
    const quota = guestQuota > 0 ? guestQuota : GUEST_GENERATION_LIMIT
    return (
      <AppLayout variant="hero">
        <div className="animate-enter flex flex-col items-center">
          <Wordmark size="hero" tagline="把一段故事，画成一幅绘本插画" />
          <div className="mt-10 w-full">
            <GuestGenerator />
            <Text className="mt-4 block text-center text-sm text-ink3">
              免费 {quota} 张 · 无需注册 · 登录后可保存到绘本并导出 PDF
            </Text>
          </div>
        </div>
      </AppLayout>
    )
  }

  const entries: Entry[] = [
    {
      key: 'books',
      title: '我的绘本',
      description: '查看已保存的绘本，逐页管理',
      to: '/books',
      icon: <BookOutlined />,
      accent: 'text-accent-leaf',
    },
    {
      key: 'profile',
      title: '个人中心',
      description: '账号信息与生成额度',
      to: '/profile',
      icon: <UserOutlined />,
      accent: 'text-accent-sky',
    },
    {
      key: 'export',
      title: '导出 PDF',
      description: '在绘本详情页一键导出多页 PDF',
      to: '/books',
      icon: <FilePdfOutlined />,
      accent: 'text-accent-plum',
    },
  ]

  return (
    <AppLayout variant="app">
      <PageHeader
        title={`${user?.username ?? ''}，欢迎回来`}
        description="输入一段故事文字，AI 帮你生成儿童绘本插画；也可把结果保存到绘本并导出多页 PDF。"
      />

      <div className="animate-enter flex flex-col items-center gap-8 py-4">
        <Button
          type="primary"
          size="large"
          shape="round"
          icon={<ThunderboltOutlined />}
          onClick={() => navigate('/generate')}
        >
          开始创作
        </Button>

        <div className="grid w-full gap-3 sm:grid-cols-3">
          {entries.map((entry) => (
            <button
              key={entry.key}
              type="button"
              onClick={() => navigate(entry.to)}
              className="rounded-card bg-paper p-5 text-left ring-1 ring-hairline transition-[transform,box-shadow] duration-200 hover:-translate-y-1 hover:shadow-lift active:scale-[0.98]"
            >
              <span className={`text-xl ${entry.accent}`}>{entry.icon}</span>
              <div className="mt-3 font-medium text-ink">{entry.title}</div>
              <Paragraph type="secondary" className="!mb-0 mt-1 text-sm">
                {entry.description}
              </Paragraph>
            </button>
          ))}
        </div>
      </div>
    </AppLayout>
  )
}

export default Home
