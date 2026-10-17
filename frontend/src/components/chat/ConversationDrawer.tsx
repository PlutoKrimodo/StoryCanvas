import { Button, Drawer, Empty, List, Popconfirm, Typography } from 'antd'
import { DeleteOutlined, MessageOutlined, PlusOutlined } from '@ant-design/icons'

import type { ConversationSummary } from '../../types/conversation'
import { formatDateTime } from '../../utils/datetime'

const { Text } = Typography

interface ConversationDrawerProps {
  open: boolean
  loading: boolean
  conversations: ConversationSummary[]
  currentId: string | null
  onClose: () => void
  onSelect: (id: string) => void
  onNew: () => void
  onDelete: (id: string) => void
}

/** 会话历史抽屉：不使用左侧导航，低干扰地从右上角展开。 */
function ConversationDrawer({
  open,
  loading,
  conversations,
  currentId,
  onClose,
  onSelect,
  onNew,
  onDelete,
}: ConversationDrawerProps) {
  return (
    <Drawer
      title="创作历史"
      placement="right"
      width={360}
      open={open}
      onClose={onClose}
      extra={
        <Button type="primary" size="small" icon={<PlusOutlined />} onClick={onNew}>
          新对话
        </Button>
      }
    >
      {conversations.length === 0 && !loading ? (
        <Empty description="还没有创作记录" />
      ) : (
        <List
          loading={loading}
          dataSource={conversations}
          renderItem={(item) => (
            <List.Item
              key={item.id}
              className={`!cursor-pointer !rounded-chip !px-3 ${
                item.id === currentId ? 'bg-canvas2 ring-1 ring-hairline' : ''
              }`}
              onClick={() => onSelect(item.id)}
              actions={[
                <Popconfirm
                  key="delete"
                  title="删除这个会话？"
                  description="相关消息会一并删除，生成图片仍保留在图片库。"
                  okText="删除"
                  cancelText="取消"
                  onConfirm={() => onDelete(item.id)}
                >
                  <Button
                    type="text"
                    size="small"
                    danger
                    icon={<DeleteOutlined />}
                    onClick={(event) => event.stopPropagation()}
                    aria-label="删除会话"
                  />
                </Popconfirm>,
              ]}
            >
              <List.Item.Meta
                avatar={<MessageOutlined className="text-clay" />}
                title={<Text strong>{item.title || '未命名对话'}</Text>}
                description={
                  <Text type="secondary" className="text-xs">
                    {formatDateTime(item.updated_at, 'MM-DD HH:mm')}
                  </Text>
                }
              />
            </List.Item>
          )}
        />
      )}
    </Drawer>
  )
}

export default ConversationDrawer
