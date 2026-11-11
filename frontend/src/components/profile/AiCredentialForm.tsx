import { Alert, AutoComplete, Button, Collapse, Input, Select, Space, Typography } from 'antd'
import { LinkOutlined } from '@ant-design/icons'

import type {
  CredentialCapability,
  CredentialCatalog,
  CredentialTestResult,
  UserAiCredential,
} from '../../types/aiCredentials'

const { Text, Link } = Typography

export interface AiCredentialFormProps {
  capability: CredentialCapability
  catalog: CredentialCatalog
  existing: UserAiCredential | null
  provider: string
  apiKey: string
  apiBase: string
  model: string
  advancedOpen: boolean
  testing: boolean
  saving: boolean
  encryptionReady: boolean
  testResult: CredentialTestResult | null
  formError: string
  onProviderChange: (value: string) => void
  onApiKeyChange: (value: string) => void
  onApiBaseChange: (value: string) => void
  onModelChange: (value: string) => void
  onAdvancedChange: (open: boolean) => void
  onTestAndSave: () => void
  onCancel: () => void
}

/**
 * BYOK 配置表单（纯展示）：厂商 → Key → 高级设置。
 *
 * 与卡片容器分离，保证单文件可读；所有敏感值只经 props 传递，
 * 不进入任何全局 store。
 */
function AiCredentialForm(props: AiCredentialFormProps) {
  const {
    capability,
    catalog,
    existing,
    provider,
    apiKey,
    apiBase,
    model,
    advancedOpen,
    testing,
    saving,
    encryptionReady,
    testResult,
    formError,
  } = props

  const preset = catalog[capability].providers.find((item) => item.id === provider)
  const modelOptions = (preset?.models ?? []).map((item) => ({
    value: item.id,
    label: item.label,
  }))

  return (
    <div aria-live="polite">
      <Text type="secondary" className="mb-4 block text-xs">
        {catalog[capability].description}
        {existing ? ` · 更新时可不重填 Key（当前 ${existing.api_key_hint}）` : ''}
      </Text>

      <Space direction="vertical" size="middle" className="w-full">
        <div className="w-full">
          <label
            className="mb-1 block text-sm text-ink"
            htmlFor={`credential-provider-${capability}`}
          >
            厂商
          </label>
          <Select
            id={`credential-provider-${capability}`}
            className="w-full"
            value={provider}
            onChange={props.onProviderChange}
            options={catalog[capability].providers.map((item) => ({
              value: item.id,
              label: item.label,
            }))}
          />
          {preset?.description ? (
            <Text type="secondary" className="mt-1 block text-xs">
              {preset.description}
            </Text>
          ) : null}
        </div>

        <div className="w-full">
          <div className="mb-1 flex flex-wrap items-center justify-between gap-2">
            <label className="text-sm text-ink" htmlFor={`credential-key-${capability}`}>
              API Key
            </label>
            {preset?.key_url ? (
              <Link href={preset.key_url} target="_blank" rel="noreferrer" className="text-xs">
                <LinkOutlined /> 去 {preset.label} 申请 Key
              </Link>
            ) : null}
          </div>
          <Input.Password
            id={`credential-key-${capability}`}
            value={apiKey}
            onChange={(event) => props.onApiKeyChange(event.target.value)}
            placeholder={
              existing ? `已保存 ${existing.api_key_hint}，留空则沿用` : '粘贴以 sk- 或 ark- 开头的 Key'
            }
            autoComplete="off"
            spellCheck={false}
          />
          <Text type="secondary" className="mt-1 block text-xs">
            Key 仅以 AES-256-GCM 密文保存在服务端，保存成功后前端不保留明文。
          </Text>
        </div>

        <Collapse
          ghost
          size="small"
          activeKey={advancedOpen ? ['advanced'] : []}
          onChange={(keys) => props.onAdvancedChange(keys.length > 0)}
          items={[
            {
              key: 'advanced',
              label: <Text type="secondary">高级设置（Base URL / 模型）</Text>,
              children: (
                <Space direction="vertical" size="small" className="w-full">
                  {capability === 'llm' ? (
                    <div className="w-full">
                      <label
                        className="mb-1 block text-xs text-ink2"
                        htmlFor={`credential-base-${capability}`}
                      >
                        Base URL
                      </label>
                      <Input
                        id={`credential-base-${capability}`}
                        value={apiBase}
                        onChange={(event) => props.onApiBaseChange(event.target.value)}
                        placeholder="https://api.deepseek.com"
                      />
                      <Text type="secondary" className="mt-1 block text-xs">
                        仅支持 HTTPS 公网域名；自定义厂商必须填写。
                      </Text>
                    </div>
                  ) : null}
                  <div className="w-full">
                    <label
                      className="mb-1 block text-xs text-ink2"
                      htmlFor={`credential-model-${capability}`}
                    >
                      模型
                    </label>
                    <AutoComplete
                      id={`credential-model-${capability}`}
                      className="w-full"
                      value={model}
                      options={modelOptions}
                      onChange={props.onModelChange}
                      placeholder={preset?.custom_base ? '例如 my-model' : '选择或输入模型名'}
                    />
                  </div>
                </Space>
              ),
            },
          ]}
        />

        {formError ? <Alert type="error" showIcon message={formError} /> : null}
        {testResult ? (
          <Alert
            type={testResult.ok ? 'success' : 'error'}
            showIcon
            message={testResult.message}
            description={
              testResult.ok
                ? testResult.verified
                  ? capability === 'image'
                    ? `已用 1 张 1K 测试图验证，耗时 ${testResult.latency_ms}ms（会计费一次）`
                    : `已用 1 token 真实探测，耗时 ${testResult.latency_ms}ms`
                  : '服务端未完成验证，不能保存；请检查配置后重试。'
                : '未通过验证，不会保存；请修正 Key 或网络后重试。'
            }
          />
        ) : null}

        {capability === 'image' ? (
          <Text type="secondary" className="block text-xs">
            保存并验证会生成一张 1K 测试图，约 10–20 秒，会计费一次。
          </Text>
        ) : null}

        <div className="flex flex-col-reverse gap-2 sm:flex-row sm:justify-end">
          {existing ? <Button onClick={props.onCancel}>取消</Button> : null}
          <Button
            type="primary"
            loading={testing || saving}
            disabled={!encryptionReady}
            onClick={props.onTestAndSave}
            block
            className="sm:w-auto"
          >
            保存并验证
          </Button>
        </div>
      </Space>
    </div>
  )
}

export default AiCredentialForm
