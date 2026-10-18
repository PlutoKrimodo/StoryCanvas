import { useCallback, useEffect, useState } from 'react'
import { Alert, App, Card, Space, Tabs, Tag, Typography } from 'antd'
import { CheckCircleFilled, KeyOutlined } from '@ant-design/icons'

import AiCredentialForm from './AiCredentialForm'
import AiCredentialSummary from './AiCredentialSummary'
import { aiCredentialsApi } from '../../api/aiCredentials'
import type {
  AiCredentialOverview,
  CredentialCapability,
  CredentialCatalog,
  CredentialTestResult,
} from '../../types/aiCredentials'

const { Paragraph } = Typography

type FormMap = Record<CredentialCapability, string>
type EditingMap = Record<CredentialCapability, boolean>

const CAPABILITIES: CredentialCapability[] = ['llm', 'image']

function emptyForm(): FormMap {
  return { llm: '', image: '' }
}

function emptyEditing(): EditingMap {
  return { llm: false, image: false }
}

function statusTag(overview: AiCredentialOverview) {
  const { llm, image } = overview.credentials
  if (llm && image) {
    return <Tag color="success">已生效 · 免每日限额</Tag>
  }
  if (llm || image) {
    return <Tag color="processing">部分生效</Tag>
  }
  return <Tag>可选</Tag>
}

interface AiCredentialCardProps {
  /** 保存或移除成功后通知外部（用于刷新模型来源与自定义张数能力） */
  onChanged?: () => void
}

/**
 * 「我的 AI API Key」配置卡片。
 *
 * 接入路径刻意压到三步：选厂商 → 粘贴 Key → 保存并验证。
 * Base URL 与模型由后端目录自动带出，仅高级设置才需要改动。
 */
function AiCredentialCard({ onChanged }: AiCredentialCardProps) {
  const { message } = App.useApp()
  const [overview, setOverview] = useState<AiCredentialOverview | null>(null)
  const [catalog, setCatalog] = useState<CredentialCatalog | null>(null)
  const [loading, setLoading] = useState(true)
  const [loadError, setLoadError] = useState('')
  const [disabled, setDisabled] = useState(false)
  const [active, setActive] = useState<CredentialCapability>('llm')

  const [provider, setProvider] = useState<FormMap>({ llm: 'deepseek', image: 'doubao' })
  const [apiKey, setApiKey] = useState<FormMap>(emptyForm)
  const [apiBase, setApiBase] = useState<FormMap>(emptyForm)
  const [model, setModel] = useState<FormMap>(emptyForm)
  const [editing, setEditing] = useState<EditingMap>(emptyEditing)
  const [advancedOpen, setAdvancedOpen] = useState(false)

  const [testing, setTesting] = useState(false)
  const [saving, setSaving] = useState(false)
  const [testResult, setTestResult] = useState<CredentialTestResult | null>(null)
  const [pendingSave, setPendingSave] = useState(false)
  const [formError, setFormError] = useState('')

  const syncForm = useCallback((cat: CredentialCatalog, ov: AiCredentialOverview) => {
    const nextProvider = { ...emptyForm() }
    const nextBase = { ...emptyForm() }
    const nextModel = { ...emptyForm() }
    CAPABILITIES.forEach((capability) => {
      const existing = ov.credentials[capability]
      const providers = cat[capability].providers
      const providerId =
        existing && providers.some((item) => item.id === existing.provider)
          ? existing.provider
          : (providers[0]?.id ?? '')
      const preset = providers.find((item) => item.id === providerId)
      nextProvider[capability] = providerId
      nextBase[capability] = existing?.api_base ?? preset?.api_base ?? ''
      nextModel[capability] = existing?.model ?? preset?.models[0]?.id ?? ''
    })
    setProvider(nextProvider)
    setApiBase(nextBase)
    setModel(nextModel)
    setEditing(emptyEditing())
    setApiKey(emptyForm())
  }, [])

  const load = useCallback(async () => {
    setLoading(true)
    setLoadError('')
    try {
      const [overviewRes, catalogRes] = await Promise.all([
        aiCredentialsApi.getOverview(),
        aiCredentialsApi.getProviders(),
      ])
      const ov = overviewRes.data.data
      const cat = catalogRes.data.data
      if (!ov || !cat) {
        throw new Error(overviewRes.data.message || '读取 API Key 配置失败')
      }
      setOverview(ov)
      setCatalog(cat)
      syncForm(cat, ov)
    } catch (error) {
      console.error('读取 API Key 配置失败', error)
      const status = (error as { response?: { status?: number } }).response?.status
      if (status === 404) {
        // BYOK_ENABLED=false：接口按设计返回 404，个人中心直接不展示配置区
        setDisabled(true)
        return
      }
      setLoadError('暂时无法读取 API Key 配置，请稍后刷新重试')
    } finally {
      setLoading(false)
    }
  }, [syncForm])

  useEffect(() => {
    void load()
  }, [load])

  const existing = overview?.credentials[active] ?? null
  const showForm = !existing || editing[active]

  const resetFeedback = () => {
    setTestResult(null)
    setPendingSave(false)
    setFormError('')
  }

  const handleProviderChange = (value: string) => {
    const nextPreset = catalog?.[active].providers.find((item) => item.id === value)
    setProvider((prev) => ({ ...prev, [active]: value }))
    setApiBase((prev) => ({ ...prev, [active]: nextPreset?.api_base ?? '' }))
    setModel((prev) => ({ ...prev, [active]: nextPreset?.models[0]?.id ?? '' }))
    // 自定义厂商必须填 Base URL / 模型：直接展开高级设置，避免用户不知道要填什么
    if (nextPreset?.custom_base) {
      setAdvancedOpen(true)
    }
    resetFeedback()
  }

  const buildPayload = () => ({
    provider: provider[active],
    api_key: apiKey[active].trim() || undefined,
    api_base: apiBase[active].trim() || undefined,
    model: model[active].trim() || undefined,
  })

  const doSave = async () => {
    setSaving(true)
    setFormError('')
    try {
      const { data } = await aiCredentialsApi.save(active, buildPayload())
      if (!data.data) {
        throw new Error(data.message || '保存失败')
      }
      message.success('API Key 已保存并生效')
      setApiKey((prev) => ({ ...prev, [active]: '' }))
      setEditing((prev) => ({ ...prev, [active]: false }))
      resetFeedback()
      await load()
      onChanged?.()
    } catch (error) {
      console.error('保存 API Key 失败', error)
      setFormError(error instanceof Error ? error.message : '保存失败，请稍后重试')
    } finally {
      setSaving(false)
    }
  }

  const handleTestAndSave = async () => {
    if (!apiKey[active].trim() && !existing) {
      setFormError('请先粘贴 API Key')
      return
    }
    if (!provider[active]) {
      setFormError('请选择厂商')
      return
    }
    setTesting(true)
    resetFeedback()
    try {
      const { data } = await aiCredentialsApi.test({
        capability: active,
        ...buildPayload(),
      })
      const result = data.data
      if (!result) {
        throw new Error(data.message || '测试失败')
      }
      setTestResult(result)
      if (result.ok) {
        await doSave()
      } else {
        setPendingSave(true)
      }
    } catch (error) {
      console.error('测试 API Key 失败', error)
      setFormError(error instanceof Error ? error.message : '测试失败，请稍后重试')
    } finally {
      setTesting(false)
    }
  }

  const handleRemove = async () => {
    try {
      await aiCredentialsApi.remove(active)
      message.success('已移除该 API Key，后续将使用系统默认配置')
      await load()
      onChanged?.()
    } catch (error) {
      console.error('移除 API Key 失败', error)
      setFormError('移除失败，请稍后重试')
    }
  }

  if (disabled) {
    return null
  }

  if (loading && !overview) {
    return (
      <Card className="shadow-card !mt-6" id="ai-credentials" loading>
        <div className="h-32" />
      </Card>
    )
  }

  if (loadError) {
    return (
      <Card className="shadow-card !mt-6" id="ai-credentials">
        <Alert type="warning" showIcon message={loadError} />
      </Card>
    )
  }

  if (!overview || !catalog) {
    return null
  }

  const summary = existing

  return (
    <Card
      className="shadow-card !mt-6"
      id="ai-credentials"
      title={
        <Space>
          <KeyOutlined className="text-clay" />
          <span>我的 AI API Key</span>
        </Space>
      }
      extra={statusTag(overview)}
    >
      <Paragraph type="secondary" className="!mb-3">
        配置后使用你自己的额度，不占全站每日限额。只配一项时，另一项仍使用系统 Key。
      </Paragraph>

      <div className="mb-4 flex flex-wrap gap-2 text-xs text-ink2">
        {['选厂商', '粘贴 Key', '保存并验证'].map((step, index) => (
          <span key={step} className="rounded-chip bg-canvas2 px-3 py-1 ring-1 ring-hairline">
            {index + 1}. {step}
          </span>
        ))}
      </div>

      {!overview.encryption_ready ? (
        <Alert
          className="mb-4"
          type="warning"
          showIcon
          message="服务端尚未配置 ENCRYPTION_KEY，暂时无法保存 API Key"
          description="请联系管理员生成密钥（openssl rand -base64 32）后重启服务。"
        />
      ) : null}

      <Tabs
        activeKey={active}
        onChange={(key) => {
          setActive(key as CredentialCapability)
          resetFeedback()
          setAdvancedOpen(false)
        }}
        items={CAPABILITIES.map((capability) => ({
          key: capability,
          label: (
            <Space size={6}>
              {catalog[capability].label}
              {overview.credentials[capability] ? (
                <CheckCircleFilled className="text-accent-leaf" />
              ) : null}
            </Space>
          ),
        }))}
      />

      {summary && !showForm ? (
        <AiCredentialSummary
          credential={summary}
          onEdit={() => setEditing((prev) => ({ ...prev, [active]: true }))}
          onRemove={() => void handleRemove()}
        />
      ) : (
        <AiCredentialForm
          capability={active}
          catalog={catalog}
          existing={existing}
          provider={provider[active]}
          apiKey={apiKey[active]}
          apiBase={apiBase[active]}
          model={model[active]}
          advancedOpen={advancedOpen}
          testing={testing}
          saving={saving}
          testResult={testResult}
          pendingSave={pendingSave}
          formError={formError}
          onProviderChange={handleProviderChange}
          onApiKeyChange={(value) => {
            setApiKey((prev) => ({ ...prev, [active]: value }))
            resetFeedback()
          }}
          onApiBaseChange={(value) => {
            setApiBase((prev) => ({ ...prev, [active]: value }))
            resetFeedback()
          }}
          onModelChange={(value) => {
            setModel((prev) => ({ ...prev, [active]: value }))
            resetFeedback()
          }}
          onAdvancedChange={setAdvancedOpen}
          onTestAndSave={() => void handleTestAndSave()}
          onSaveAnyway={() => void doSave()}
          onCancel={() => {
            setEditing((prev) => ({ ...prev, [active]: false }))
            setApiKey((prev) => ({ ...prev, [active]: '' }))
            resetFeedback()
          }}
        />
      )}
    </Card>
  )
}

export default AiCredentialCard
