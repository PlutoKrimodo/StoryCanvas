import { Avatar, Button, Card, Typography } from 'antd'
import { UserOutlined } from '@ant-design/icons'

import AppLayout from '../../components/layout/AppLayout'
import PageHeader from '../../components/layout/PageHeader'
import { useAuth } from '../../hooks/useAuth'
import { formatDateTime } from '../../utils/datetime'

const { Title } = Typography

function Profile() {
  const { user, signOut } = useAuth()

  return (
    <AppLayout variant="app">
      <div className="mx-auto w-full max-w-2xl">
        <PageHeader title="个人中心" description="账号信息与登录状态" />

        <Card className="shadow-card">
          <div className="flex items-center gap-4 mb-6">
            <Avatar
              size={64}
              src={user?.avatar || undefined}
              icon={<UserOutlined />}
              className="ring-2 ring-clay/30"
            />
            <div>
              <Title level={4} className="!mb-0">
                {user?.username ?? '未登录'}
              </Title>
            </div>
          </div>

          <ul className="divide-y divide-hairline border-t border-hairline">
            {(
              [
                ['用户 ID', user?.id ?? '-'],
                ['用户名', user?.username ?? '-'],
                ['邮箱', user?.email ?? '-'],
                ['注册时间', formatDateTime(user?.created_at, 'YYYY-MM-DD HH:mm:ss')],
              ] as const
            ).map(([label, value]) => (
              <li key={label} className="flex items-baseline justify-between gap-6 py-3">
                <span className="shrink-0 text-sm text-ink2">{label}</span>
                <span className="min-w-0 break-all text-sm text-ink">{value}</span>
              </li>
            ))}
          </ul>

          <div className="mt-6 text-right">
            <Button danger onClick={signOut}>
              退出登录
            </Button>
          </div>
        </Card>
      </div>
    </AppLayout>
  )
}

export default Profile
