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
  testResult: CredentialTestResult | null
  pendingSave: boolean
  formError: string
  onProviderChange: (value: string) => void
  onApiKeyChange: (value: string) => void
  onApiBaseChange: (value: string) => void
  onModelChange: (value: string) => void
  onAdvancedChange: (open: boolean) => void
  onTestAndSave: () => void
  onSaveAnyway: () => void
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
    testResult,
    pendingSave,
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
                  ? `已用 1 token 真实探测，耗时 ${testResult.latency_ms}ms`
                  : '图像配置仅做格式校验，首次生成时才会真正调用厂商'
                : '你可以修正后重试，也可以仍然保存（生成时会再次验证）'
            }
            action={
              !testResult.ok && pendingSave ? (
                <Button size="small" loading={saving} onClick={props.onSaveAnyway}>
                  仍然保存
                </Button>
              ) : null
            }
          />
        ) : null}

        <div className="flex flex-col-reverse gap-2 sm:flex-row sm:justify-end">
          {existing ? <Button onClick={props.onCancel}>取消</Button> : null}
          <Button
            type="primary"
            loading={testing || saving}
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
