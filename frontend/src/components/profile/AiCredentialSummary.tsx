import { Button, Divider, Popconfirm, Typography } from 'antd'
import { DeleteOutlined, EditOutlined } from '@ant-design/icons'

import type { UserAiCredential } from '../../types/aiCredentials'
import { formatDateTime } from '../../utils/datetime'

const { Text } = Typography

interface AiCredentialSummaryProps {
  credential: UserAiCredential
  onEdit: () => void
  onRemove: () => void
}

/** 已配置状态的紧凑摘要：脱敏 Key、模型、更新时间与更新/移除入口。 */
function AiCredentialSummary({ credential, onEdit, onRemove }: AiCredentialSummaryProps) {
  return (
    <div>
      <div className="rounded-card bg-canvas2 px-4 py-3 ring-1 ring-hairline">
        <div className="flex flex-wrap items-center justify-between gap-2">
          <Text strong>{credential.provider_label}</Text>
          <Text code>{credential.api_key_hint}</Text>
        </div>
        <div className="mt-2 flex flex-wrap gap-x-4 gap-y-1 text-xs text-ink2">
          <span>模型：{credential.model ?? '厂商默认'}</span>
          <span>更新于 {formatDateTime(credential.updated_at, 'YYYY-MM-DD HH:mm')}</span>
        </div>
      </div>
      <Divider className="!my-4" />
      <div className="flex flex-wrap justify-end gap-2">
        <Popconfirm
          title="移除后该能力将回到系统默认配置"
          description="已保存的 Key 会被删除，且无法恢复。"
          okText="确认移除"
          cancelText="取消"
          onConfirm={onRemove}
        >
          <Button danger icon={<DeleteOutlined />}>
            移除
          </Button>
        </Popconfirm>
        <Button type="primary" icon={<EditOutlined />} onClick={onEdit}>
          更新
        </Button>
      </div>
    </div>
  )
}

export default AiCredentialSummary
