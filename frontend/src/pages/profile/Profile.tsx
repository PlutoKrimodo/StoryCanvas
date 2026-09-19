import { Avatar, Button, Card, Descriptions, Typography } from 'antd'
import { UserOutlined } from '@ant-design/icons'

import { useAuth } from '../../hooks/useAuth'

const { Title } = Typography

function Profile() {
  const { user, signOut } = useAuth()

  return (
    <div className="min-h-screen bg-gray-50 px-4 py-10">
      <div className="max-w-2xl mx-auto">
        <Card>
          <div className="flex items-center gap-4 mb-6">
            <Avatar size={64} src={user?.avatar || undefined} icon={<UserOutlined />} />
            <div>
              <Title level={4} className="!mb-0">
                {user?.username ?? '未登录'}
              </Title>
              <Typography.Text type="secondary">{user?.email}</Typography.Text>
            </div>
          </div>

          <Descriptions column={1} bordered size="middle">
            <Descriptions.Item label="用户 ID">{user?.id ?? '-'}</Descriptions.Item>
            <Descriptions.Item label="用户名">{user?.username ?? '-'}</Descriptions.Item>
            <Descriptions.Item label="邮箱">{user?.email ?? '-'}</Descriptions.Item>
            <Descriptions.Item label="注册时间">
              {user?.created_at ?? '-'}
            </Descriptions.Item>
          </Descriptions>

          <div className="mt-6 text-right">
            <Button danger onClick={signOut}>
              退出登录
            </Button>
          </div>
        </Card>
      </div>
    </div>
  )
}

export default Profile
