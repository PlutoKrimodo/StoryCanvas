import { Button, Card, Typography } from 'antd'
import { useNavigate } from 'react-router-dom'

import { useAuth } from '../../hooks/useAuth'

const { Title, Paragraph, Text } = Typography

function Home() {
  const navigate = useNavigate()
  const { user, signOut } = useAuth()

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-purple-50">
      <div className="container mx-auto px-4 py-16">
        <Title level={1} className="!text-center !mb-3">
          StoryCanvas
        </Title>
        <Paragraph className="text-center text-lg text-gray-600">
          基于生成式 AI 的儿童绘本智能创作与图像生成系统
        </Paragraph>

        <div className="flex justify-center mt-10">
          <Card className="w-full max-w-lg">
            <Paragraph className="!mb-2">
              <Text strong>当前用户：</Text>
              {user?.username ?? '未登录'}
            </Paragraph>
            <Paragraph type="secondary">
              用户管理子系统已就绪；智能图像生成子系统将在后续里程碑接入。
            </Paragraph>

            <div className="flex gap-3 mt-6">
              <Button type="primary" onClick={() => navigate('/profile')}>
                个人中心
              </Button>
              <Button onClick={signOut}>退出登录</Button>
            </div>
          </Card>
        </div>
      </div>
    </div>
  )
}

export default Home
