import { cleanup, fireEvent, screen, waitFor } from '@testing-library/react'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import AiCredentialCard from './AiCredentialCard'
import { renderWithTheme } from '../../test/renderWithTheme'

const api = vi.hoisted(() => ({
  getOverview: vi.fn(),
  getProviders: vi.fn(),
  save: vi.fn(),
  remove: vi.fn(),
  test: vi.fn(),
}))

vi.mock('../../api/aiCredentials', () => ({ aiCredentialsApi: api }))

const catalog = {
  llm: {
    label: '文本理解',
    description: '把故事解析为角色、场景、动作等结构化数据',
    providers: [
      {
        id: 'deepseek',
        label: 'DeepSeek',
        description: '性价比高，中文理解稳定',
        api_base: 'https://api.deepseek.com',
        key_url: 'https://platform.deepseek.com/api_keys',
        custom_base: false,
        models: [
          { id: 'deepseek-chat', label: 'DeepSeek Chat' },
          { id: 'deepseek-reasoner', label: 'DeepSeek Reasoner' },
        ],
      },
    ],
  },
  image: {
    label: '图像生成',
    description: '根据故事生成绘本插画',
    providers: [
      {
        id: 'doubao',
        label: '豆包 · Seedream',
        description: '火山方舟，画面质感好',
        api_base: 'https://ark.cn-beijing.volces.com/api/v3',
        key_url: 'https://console.volcengine.com/ark',
        custom_base: false,
        models: [{ id: 'doubao-seedream-5-0-flash-260915', label: 'Seedream 5.0 Flash' }],
      },
    ],
  },
}

function overviewWithLlm(configured: boolean) {
  return {
    byok_enabled: true,
    encryption_ready: true,
    quota_exempt: false,
    credentials: {
      llm: configured
        ? {
            capability: 'llm',
            capability_label: '文本理解',
            provider: 'deepseek',
            provider_label: 'DeepSeek',
            api_key_hint: 'sk-****abcd',
            api_base: 'https://api.deepseek.com',
            model: 'deepseek-chat',
            updated_at: '2026-10-17T10:00:00Z',
          }
        : null,
      image: null,
    },
  }
}

function mockResolved(configured = false) {
  api.getOverview.mockResolvedValue({ data: { data: overviewWithLlm(configured) } })
  api.getProviders.mockResolvedValue({ data: { data: catalog } })
  api.save.mockResolvedValue({
    data: { data: overviewWithLlm(true).credentials.llm, message: 'ok' },
  })
  api.remove.mockResolvedValue({ data: { data: null } })
  api.test.mockResolvedValue({
    data: { data: { ok: true, verified: true, latency_ms: 120, message: '连接成功' } },
  })
}

describe('AiCredentialCard（BYOK 接入）', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    mockResolved(false)
  })

  afterEach(() => {
    cleanup()
  })

  it('未配置时展示三步引导与默认厂商', async () => {
    renderWithTheme(<AiCredentialCard />)

    expect(await screen.findByText('我的 AI API Key')).toBeInTheDocument()
    expect(screen.getByText('1. 选厂商')).toBeInTheDocument()
    expect(screen.getByText('可选')).toBeInTheDocument()
    expect(screen.getByLabelText('API Key')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: '保存并验证' })).toBeInTheDocument()
  })

  it('保存并验证：先测试再保存，成功后清空输入', async () => {
    renderWithTheme(<AiCredentialCard />)
    await screen.findByText('我的 AI API Key')

    fireEvent.change(screen.getByLabelText('API Key'), {
      target: { value: 'sk-new-key-123456' },
    })
    fireEvent.click(screen.getByRole('button', { name: '保存并验证' }))

    await waitFor(() => expect(api.test).toHaveBeenCalled())
    await waitFor(() =>
      expect(api.save).toHaveBeenCalledWith(
        'llm',
        expect.objectContaining({
          provider: 'deepseek',
          api_key: 'sk-new-key-123456',
          api_base: 'https://api.deepseek.com',
          model: 'deepseek-chat',
        }),
      ),
    )
  })

  it('测试失败时提供「仍然保存」而不是阻塞用户', async () => {
    api.test.mockResolvedValue({
      data: {
        data: { ok: false, verified: false, latency_ms: 0, message: 'API Key 无效（HTTP 401）' },
      },
    })

    renderWithTheme(<AiCredentialCard />)
    await screen.findByText('我的 AI API Key')
    fireEvent.change(screen.getByLabelText('API Key'), {
      target: { value: 'sk-bad-key-123456' },
    })
    fireEvent.click(screen.getByRole('button', { name: '保存并验证' }))

    expect(await screen.findByText('API Key 无效（HTTP 401）')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: '仍然保存' })).toBeInTheDocument()
  })

  it('已配置时脱敏展示，并支持展开高级设置查看自动带出的 Base URL', async () => {
    mockResolved(true)
    renderWithTheme(<AiCredentialCard />)

    expect(await screen.findByText('sk-****abcd')).toBeInTheDocument()
    expect(screen.getByText('部分生效')).toBeInTheDocument()

    fireEvent.click(screen.getByText('更新'))
    fireEvent.click(screen.getByText('高级设置（Base URL / 模型）'))

    expect(await screen.findByDisplayValue('https://api.deepseek.com')).toBeInTheDocument()
  })
})
