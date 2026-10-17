import { useEffect, useState } from 'react'
import { Button, Input, Space, Tag, Typography } from 'antd'
import { PlayCircleOutlined, ReloadOutlined } from '@ant-design/icons'

import type { ChatMessage, StoryboardPanel } from '../../types/conversation'

const { Text, Paragraph } = Typography
const { TextArea } = Input

interface StoryboardMessageProps {
  message: ChatMessage
  originalText?: string
  disabled?: boolean
  onStart: (panels: StoryboardPanel[]) => void
  onResplit?: (text: string) => void
}

/** AI 分镜消息：逐格可编辑，确认后才产生图像费用。 */
function StoryboardMessage({
  message,
  originalText,
  disabled = false,
  onStart,
  onResplit,
}: StoryboardMessageProps) {
  const [panels, setPanels] = useState<StoryboardPanel[]>(message.payload.panels ?? [])
  const draft = (message.payload.status ?? 'draft') === 'draft'

  useEffect(() => {
    setPanels(message.payload.panels ?? [])
  }, [message.payload.panels])

  const consistency = message.payload.consistency
  const characters = (consistency?.characters ?? []).filter((item) => item.name)

  const updatePanel = (index: number, text: string) => {
    setPanels((prev) =>
      prev.map((panel) => (panel.index === index ? { ...panel, text } : panel)),
    )
  }

  return (
    <div className="rounded-card border border-hairline bg-canvas2/70 p-4">
      <div className="mb-3 flex flex-wrap items-start justify-between gap-2">
        <div>
          <Text strong>{message.content || '分镜提案'}</Text>
          <Paragraph type="secondary" className="!mb-0 mt-1 text-xs">
            确认每格描述后开始绘制；确认前不会产生图像费用。
          </Paragraph>
        </div>
        {characters.length ? (
          <Space size={[4, 4]} wrap>
            {characters.map((item) => (
              <Tag key={item.name} color="volcano">
                {item.name}
                {item.appearance ? ` · ${item.appearance}` : ''}
              </Tag>
            ))}
          </Space>
        ) : null}
      </div>

      <div className="grid gap-3 sm:grid-cols-2">
        {panels.map((panel) => (
          <div
            key={panel.index}
            className="rounded-chip border border-hairline bg-paper p-3"
          >
            <div className="mb-1 flex items-center justify-between">
              <Tag color="processing">第 {panel.index + 1} 格</Tag>
              <Text type="secondary" className="text-xs">
                {panel.text.length}/500
              </Text>
            </div>
            <TextArea
              aria-label={`第 ${panel.index + 1} 格描述`}
              value={panel.text}
              maxLength={500}
              autoSize={{ minRows: 2, maxRows: 4 }}
              disabled={!draft || disabled}
              onChange={(event) => updatePanel(panel.index, event.target.value)}
            />
          </div>
        ))}
      </div>

      {consistency?.palette || consistency?.notes ? (
        <Text type="secondary" className="mt-3 block text-xs">
          {consistency.palette ? `色调：${consistency.palette}　` : ''}
          {consistency.notes ?? ''}
        </Text>
      ) : null}

      {draft ? (
        <div className="mt-4 flex flex-wrap justify-end gap-2">
          {onResplit && originalText ? (
            <Button
              icon={<ReloadOutlined />}
              disabled={disabled}
              onClick={() => onResplit(originalText)}
            >
              重新拆分
            </Button>
          ) : null}
          <Button
            type="primary"
            icon={<PlayCircleOutlined />}
            loading={disabled}
            disabled={panels.some((panel) => !panel.text.trim())}
            onClick={() => onStart(panels)}
          >
            开始画这 {panels.length} 张
          </Button>
        </div>
      ) : (
        <Text type="secondary" className="mt-3 block text-xs">
          分镜已提交，正在生成图片。
        </Text>
      )}
    </div>
  )
}

export default StoryboardMessage
