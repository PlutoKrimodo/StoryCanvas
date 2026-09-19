import { Button, Card, Form, Input, Typography } from 'antd'
import { LockOutlined, MailOutlined } from '@ant-design/icons'
import { Link } from 'react-router-dom'

import { useAuth } from '../../hooks/useAuth'
import type { LoginRequest } from '../../types/auth'

const { Title, Text } = Typography

function Login() {
  const { loginWithPassword, loading } = useAuth()

  const handleFinish = async (values: LoginRequest) => {
    try {
      await loginWithPassword(values)
    } catch {
      // 错误提示已由响应拦截器统一给出（登录失败不区分账号是否存在）
    }
  }

  return (
    <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-blue-50 to-purple-50 px-4">
      <Card className="w-full max-w-md shadow-lg">
        <div className="text-center mb-6">
          <Title level={3} className="!mb-1">
            登录 StoryCanvas
          </Title>
          <Text type="secondary">基于生成式 AI 的儿童绘本创作平台</Text>
        </div>

        <Form<LoginRequest> layout="vertical" onFinish={handleFinish} requiredMark={false}>
          <Form.Item
            name="email"
            label="邮箱"
            rules={[
              { required: true, message: '请输入邮箱' },
              { type: 'email', message: '邮箱格式不正确' },
            ]}
          >
            <Input prefix={<MailOutlined />} placeholder="you@example.com" autoComplete="email" />
          </Form.Item>

          <Form.Item
            name="password"
            label="密码"
            rules={[{ required: true, message: '请输入密码' }]}
          >
            <Input.Password
              prefix={<LockOutlined />}
              placeholder="请输入密码"
              autoComplete="current-password"
            />
          </Form.Item>

          <Form.Item className="!mb-2">
            <Button type="primary" htmlType="submit" block loading={loading}>
              登录
            </Button>
          </Form.Item>
        </Form>

        <div className="text-center">
          <Text type="secondary">还没有账号？</Text> <Link to="/register">立即注册</Link>
        </div>
      </Card>
    </div>
  )
}

export default Login
