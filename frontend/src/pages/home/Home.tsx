import { Button, Card, Typography } from 'antd'
import { useNavigate } from 'react-router-dom'

import GuestGenerator from '../../components/ai/GuestGenerator'
import { useAuth } from '../../hooks/useAuth'

const { Title, Paragraph, Text } = Typography

function Home() {
  const navigate = useNavigate()
  const { user, isAuthenticated, signOut } = useAuth()

  // Guest-first：未登录用户进入首页即看到生成器，可先体验后注册
  if (!isAuthenticated) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-[#F5F3FF] via-white to-[#EEF2FF]">
        <div className="max-w-2xl mx-auto px-4 py-12">
          <div className="text-center mb-8">
            <Title level={1} className="!mb-3">
              StoryCanvas
            </Title>
            <Paragraph className="text-lg text-gray-600">
              基于生成式 AI 的儿童绘本智能创作与图像生成系统
            </Paragraph>
            <div className="flex justify-center gap-3 mt-4">
              <Button type="primary" onClick={() => navigate('/login')}>
                登录
              </Button>
              <Button onClick={() => navigate('/register')}>
                注册
              </Button>
            </div>
          </div>

          <GuestGenerator />
        </div>
      </div>
    )
  }

  // 已登录：工作台
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
              输入一段故事文字，AI 帮你生成儿童绘本插画；也可把结果保存到绘本并导出多页 PDF。
            </Paragraph>

            <div className="flex flex-wrap gap-3 mt-6">
              <Button type="primary" onClick={() => navigate('/generate')}>
                AI 生成插画
              </Button>
              <Button onClick={() => navigate('/books')}>我的绘本</Button>
              <Button onClick={() => navigate('/profile')}>个人中心</Button>
              <Button onClick={signOut}>退出登录</Button>
            </div>
          </Card>
        </div>
      </div>
    </div>
  )
}

export default Home
