import { Button, Card, Form, Input, Typography, message } from 'antd'
import { LockOutlined, MailOutlined, UserOutlined } from '@ant-design/icons'
import { Link, useNavigate } from 'react-router-dom'

import { useAuth } from '../../hooks/useAuth'
import type { RegisterRequest } from '../../types/auth'

const { Title, Text } = Typography

interface RegisterForm extends RegisterRequest {
  confirm: string
}

function Register() {
  const navigate = useNavigate()
  const { register, loading } = useAuth()

  const handleFinish = async (values: RegisterForm) => {
    try {
      await register({
        username: values.username,
        email: values.email,
        password: values.password,
      })
      message.success('注册成功，请登录')
      navigate('/login', { replace: true })
    } catch {
      // 错误提示已由响应拦截器统一给出
    }
  }

  return (
    <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-blue-50 to-purple-50 px-4">
      <Card className="w-full max-w-md shadow-lg">
        <div className="text-center mb-6">
          <Title level={3} className="!mb-1">
            注册账号
          </Title>
          <Text type="secondary">创建你的 StoryCanvas 账号</Text>
        </div>

        <Form<RegisterForm> layout="vertical" onFinish={handleFinish} requiredMark={false}>
          <Form.Item
            name="username"
            label="用户名"
            rules={[
              { required: true, message: '请输入用户名' },
              { min: 3, max: 50, message: '用户名长度为 3-50 个字符' },
              { pattern: /^[a-zA-Z0-9_]+$/, message: '仅支持字母、数字和下划线' },
            ]}
          >
            <Input prefix={<UserOutlined />} placeholder="3-50 位字母、数字或下划线" />
          </Form.Item>

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
            rules={[
              { required: true, message: '请输入密码' },
              { min: 8, message: '密码至少 8 位' },
              {
                pattern: /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d).+$/,
                message: '需同时包含大小写字母和数字',
              },
            ]}
          >
            <Input.Password
              prefix={<LockOutlined />}
              placeholder="至少 8 位，含大小写字母和数字"
              autoComplete="new-password"
            />
          </Form.Item>

          <Form.Item
            name="confirm"
            label="确认密码"
            dependencies={['password']}
            rules={[
              { required: true, message: '请再次输入密码' },
              ({ getFieldValue }) => ({
                validator(_, value) {
                  if (!value || getFieldValue('password') === value) {
                    return Promise.resolve()
                  }
                  return Promise.reject(new Error('两次输入的密码不一致'))
                },
              }),
            ]}
          >
            <Input.Password
              prefix={<LockOutlined />}
              placeholder="请再次输入密码"
              autoComplete="new-password"
            />
          </Form.Item>

          <Form.Item className="!mb-2">
            <Button type="primary" htmlType="submit" block loading={loading}>
              注册
            </Button>
          </Form.Item>
        </Form>

        <div className="text-center">
          <Text type="secondary">已有账号？</Text> <Link to="/login">返回登录</Link>
        </div>
      </Card>
    </div>
  )
}

export default Register
