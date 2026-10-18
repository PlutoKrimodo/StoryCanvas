import { useState } from 'react'
import { Button, Card, Form, Input, Typography } from 'antd'
import { LockOutlined, MailOutlined, UserOutlined } from '@ant-design/icons'
import { Link } from 'react-router-dom'

import AppLayout from '../../components/layout/AppLayout'
import { useAuth } from '../../hooks/useAuth'
import type { LoginRequest } from '../../types/auth'

const { Title, Text } = Typography

type LoginMode = 'email' | 'username'

interface LoginFormValues {
  email?: string
  username?: string
  password: string
}

function Login() {
  const { loginWithPassword, loading } = useAuth()
  const [mode, setMode] = useState<LoginMode>('email')

  const handleFinish = async (values: LoginFormValues) => {
    const identifier = (mode === 'email' ? values.email : values.username)?.trim()
    if (!identifier) return

    const payload: LoginRequest = {
      email: identifier,
      password: values.password,
      login_type: mode,
    }
    try {
      await loginWithPassword(payload)
    } catch {
      // 错误提示已由响应拦截器统一给出（登录失败不区分账号是否存在）
    }
  }

  const tabClass = (active: boolean) =>
    `flex items-center justify-center gap-2 rounded-card border px-3 py-2.5 text-sm transition-all duration-200 ${
      active
        ? 'border-clay bg-clay/5 font-medium text-ink shadow-card'
        : 'border-hairline bg-paper text-ink2 hover:border-clay/40 hover:bg-canvas2'
    }`

  return (
    <AppLayout variant="auth">
      <Card className="shadow-card">
        <div className="mb-6 text-center">
          <Title level={3} className="!mb-1">
            登录 StoryCanvas
          </Title>
          <Text type="secondary">基于生成式 AI 的儿童绘本创作平台</Text>
        </div>

        <div role="tablist" aria-label="登录方式" className="mb-5 grid grid-cols-2 gap-2">
          <button
            type="button"
            role="tab"
            aria-selected={mode === 'email'}
            onClick={() => setMode('email')}
            className={tabClass(mode === 'email')}
          >
            <MailOutlined /> 邮箱登录
          </button>
          <button
            type="button"
            role="tab"
            aria-selected={mode === 'username'}
            onClick={() => setMode('username')}
            className={tabClass(mode === 'username')}
          >
            <UserOutlined /> 用户名登录
          </button>
        </div>

        <Form<LoginFormValues> layout="vertical" onFinish={handleFinish} requiredMark={false}>
          {mode === 'email' ? (
            <Form.Item
              name="email"
              label="邮箱"
              rules={[
                { required: true, message: '请输入邮箱' },
                { type: 'email', message: '邮箱格式不正确' },
              ]}
            >
              <Input
                prefix={<MailOutlined />}
                placeholder="you@example.com"
                autoComplete="email"
              />
            </Form.Item>
          ) : (
            <Form.Item
              name="username"
              label="用户名"
              rules={[
                { required: true, message: '请输入用户名' },
                { max: 50, message: '用户名不能超过 50 个字符' },
              ]}
            >
              <Input
                prefix={<UserOutlined />}
                placeholder="请输入用户名"
                autoComplete="username"
              />
            </Form.Item>
          )}

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
    </AppLayout>
  )
}

export default Login
