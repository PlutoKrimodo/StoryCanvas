import { useCallback, useEffect, useState, type ReactNode } from 'react'
import { Alert, App, Button, Modal, Space, Spin, Tag, Typography } from 'antd'
import { CheckCircleFilled, CloudOutlined, RobotOutlined } from '@ant-design/icons'

import AiCredentialCard from '../profile/AiCredentialCard'
import { aiCredentialsApi } from '../../api/aiCredentials'
import type { AiCredentialOverview } from '../../types/aiCredentials'

const { Text } = Typography

type ModelMode = 'default' | 'custom'

export interface ModelManagerModalProps {
  open: boolean
  onClose: () => void
  /** 模型配置发生变化（保存/移除/切换默认）后通知父级刷新 byokDual */
  onChanged: () => void
}

interface ModeCardProps {
  active: boolean
  disabled?: boolean
  title: string
  description: string
  icon: ReactNode
  onClick: () => void
}

function ModeCard({ active, disabled, title, description, icon, onClick }: ModeCardProps) {
  return (
    <button
      type="button"
      role="radio"
      aria-checked={active}
      disabled={disabled}
      onClick={onClick}
      className={`flex items-start gap-3 rounded-card border p-4 text-left transition-all duration-200 ${
        active
          ? 'border-clay bg-clay/5 shadow-card'
          : 'border-hairline bg-paper hover:border-clay/40 hover:shadow-card'
      } ${disabled ? 'cursor-not-allowed opacity-50' : 'cursor-pointer'}`}
    >
      <span
        className={`mt-0.5 flex h-8 w-8 shrink-0 items-center justify-center rounded-full ${
          active ? 'bg-clay text-white' : 'bg-canvas2 text-ink2'
        }`}
      >
        {icon}
      </span>
      <span className="min-w-0">
        <span className="flex flex-wrap items-center gap-2">
          <span className="font-medium text-ink">{title}</span>
          {active ? <Tag color="success">当前</Tag> : null}
        </span>
        <span className="mt-1 block text-xs leading-relaxed text-ink2">{description}</span>
      </span>
    </button>
  )
}

/**
 * 模型管理弹窗。
 *
 * 默认模型 = 系统内置 Key（无需配置）；模型自定义 = 用户 BYOK。
 * 由于后端以「凭据是否存在」作为 BYOK 开关，切回默认模型必须在用户确认后
 * 移除已保存的凭据，不能静默删除。
 */
function ModelManagerModal({ open, onClose, onChanged }: ModelManagerModalProps) {
  const { modal, message } = App.useApp()
  const [mode, setMode] = useState<ModelMode>('default')
  const [overview, setOverview] = useState<AiCredentialOverview | null>(null)
  const [loading, setLoading] = useState(false)
  const [switching, setSwitching] = useState(false)
  const [error, setError] = useState('')

  const loadOverview = useCallback(async () => {
    const { data } = await aiCredentialsApi.getOverview()
    const value = data.data
    if (!value) throw new Error(data.message || '读取模型配置失败')
    setOverview(value)
    return value
  }, [])

  useEffect(() => {
    if (!open) return
    setLoading(true)
    setError('')
    void loadOverview()
      .then((value) => {
        setMode(value.credentials.llm || value.credentials.image ? 'custom' : 'default')
      })
      .catch((err) => {
        console.error('读取模型配置失败', err)
        const status = (err as { response?: { status?: number } }).response?.status
        setError(
          status === 404
            ? '当前服务未启用自定义模型功能，将使用系统内置模型'
            : '暂时无法读取模型配置，请稍后重试',
        )
        setMode('default')
      })
      .finally(() => setLoading(false))
  }, [open, loadOverview])

  const llm = overview?.credentials.llm ?? null
  const image = overview?.credentials.image ?? null
  const hasAny = Boolean(llm || image)
  const hasBoth = Boolean(llm && image)

  const switchToDefault = () => {
    if (!hasAny) {
      setMode('default')
      return
    }
    modal.confirm({
      title: '切换到默认模型？',
      content:
        '已保存的自定义 Key 将被移除，之后使用系统内置模型，并计入全站每日生成限额。此操作不可撤销。',
      okText: '切换并移除',
      cancelText: '取消',
      okButtonProps: { danger: true },
      onOk: async () => {
        setSwitching(true)
        try {
          const removals: Array<Promise<unknown>> = []
          if (llm) removals.push(aiCredentialsApi.remove('llm'))
          if (image) removals.push(aiCredentialsApi.remove('image'))
          await Promise.all(removals)
          const value = await loadOverview()
          setMode(value.credentials.llm || value.credentials.image ? 'custom' : 'default')
          message.success('已切换为系统默认模型')
          onChanged()
        } catch (err) {
          console.error('切换默认模型失败', err)
          message.error('切换失败，请稍后重试')
        } finally {
          setSwitching(false)
        }
      },
    })
  }

  const handleModeChange = (next: ModelMode) => {
    if (error) return
    if (next === 'custom') {
      setMode('custom')
      return
    }
    switchToDefault()
  }

  const handleCredentialChanged = () => {
    setMode('custom')
    void loadOverview()
      .then((value) => {
        setMode(value.credentials.llm || value.credentials.image ? 'custom' : 'default')
      })
      .catch((err) => console.error('刷新模型配置失败', err))
    onChanged()
  }

  return (
    <Modal
      open={open}
      onCancel={onClose}
      footer={null}
      centered
      width={760}
      maskClosable
      title={
        <Space>
          <RobotOutlined className="text-clay" />
          <span>模型管理</span>
        </Space>
      }
      styles={{
        mask: {
          backdropFilter: 'blur(8px)',
          backgroundColor: 'rgba(43,38,33,0.32)',
        },
        body: { maxHeight: '70vh', overflowY: 'auto' },
      }}
    >
      {loading ? (
        <div className="flex items-center justify-center py-12">
          <Spin />
        </div>
      ) : (
        <div className="flex flex-col gap-4">
          {error ? <Alert type="warning" showIcon message={error} /> : null}

          <div role="radiogroup" aria-label="模型来源" className="grid gap-3 sm:grid-cols-2">
            <ModeCard
              active={mode === 'default'}
              title="默认模型"
              description="使用系统内置的 DeepSeek 与豆包 / 通义万相，无需配置，生成计入全站每日限额。"
              icon={<CloudOutlined />}
              onClick={() => handleModeChange('default')}
            />
            <ModeCard
              active={mode === 'custom'}
              disabled={Boolean(error)}
              title="模型自定义"
              description="填写自己的文本 / 图像 API Key；两项都配置后不占全站限额，并可自定义图片张数。"
              icon={<RobotOutlined />}
              onClick={() => handleModeChange('custom')}
            />
          </div>

          {mode === 'default' ? (
            <div className="rounded-card border border-hairline bg-canvas2/60 p-4">
              <Space direction="vertical" size={8} className="w-full">
                <Text strong>系统内置模型</Text>
                <Text type="secondary" className="text-xs leading-relaxed">
                  文本理解：DeepSeek；图像生成：豆包 · Seedream / 通义万相（主备自动降级）。
                </Text>
                <Text type="secondary" className="text-xs leading-relaxed">
                  生成会计入全站每日限额；配置自定义模型后不再占用。
                </Text>
                {hasAny ? (
                  <Alert
                    type="warning"
                    showIcon
                    message="当前仍有自定义模型配置"
                    description="选择「默认模型」并确认后，已保存的自定义 Key 会被移除。"
                    action={
                      <Button size="small" danger loading={switching} onClick={switchToDefault}>
                        切换为默认模型
                      </Button>
                    }
                  />
                ) : (
                  <Text className="flex items-center gap-1 text-xs text-accent-leaf">
                    <CheckCircleFilled />
                    当前正在使用系统内置模型
                  </Text>
                )}
              </Space>
            </div>
          ) : (
            <div className="flex flex-col gap-3">
              <Alert
                type={hasBoth ? 'success' : 'info'}
                showIcon
                message={
                  hasBoth
                    ? '已启用双项自定义模型：不占全站限额，图片张数可自定义（最多 12 张）'
                    : '分别配置文本与图像两项后，生成不占全站限额，并可自定义图片张数'
                }
              />
              <AiCredentialCard onChanged={handleCredentialChanged} />
            </div>
          )}
        </div>
      )}
    </Modal>
  )
}

export default ModelManagerModal
