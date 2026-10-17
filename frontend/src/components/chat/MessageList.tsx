import { useEffect, useRef } from 'react'
import { Spin, Typography } from 'antd'

import ImageGridMessage from './ImageGridMessage'
import StoryboardMessage from './StoryboardMessage'
import type { BatchItem, BatchView, ChatMessage, StoryboardPanel } from '../../types/conversation'

const { Text } = Typography

interface MessageListProps {
  messages: ChatMessage[]
  batch: BatchView | null
  polling: boolean
  sending: boolean
  onStart: (message: ChatMessage, panels: StoryboardPanel[]) => void
  onResplit: (text: string) => void
  onDownload: (imageId: string) => void
  onSave: (imageId: string) => void
  onSaveAll: (items: BatchItem[]) => void
  onRegenerate: (panelIndex: number, batchId?: string) => void
  onCancel: (batchId?: string) => void
}

function itemsOf(message: ChatMessage, batch: BatchView | null): BatchItem[] {
  if (message.payload.images?.length) {
    return message.payload.images
  }
  if (batch && message.payload.batch_id === batch.batch_id) {
    return batch.items ?? []
  }
  return []
}

/** 对话消息流：用户右、AI 左；分镜与图片消息按状态切换渲染。 */
function MessageList(props: MessageListProps) {
  const endRef = useRef<HTMLDivElement | null>(null)

  useEffect(() => {
    endRef.current?.scrollIntoView({ behavior: 'smooth', block: 'end' })
  }, [props.messages.length, props.batch?.status, props.batch?.items?.length])

  const originalTextBefore = (index: number): string | undefined => {
    for (let cursor = index - 1; cursor >= 0; cursor -= 1) {
      const message = props.messages[cursor]
      if (message.role === 'user' && message.content) {
        return message.content
      }
    }
    return undefined
  }

  return (
    <div className="flex flex-col gap-5">
      {props.messages.map((message, index) => {
        if (message.role === 'user') {
          return (
            <div key={message.id} className="flex animate-enter justify-end">
              <div className="max-w-[85%] rounded-card rounded-br-md bg-clay px-4 py-2.5 text-sm leading-relaxed text-white shadow-card">
                {message.content}
              </div>
            </div>
          )
        }

        const items = itemsOf(message, props.batch)
        const batchActive =
          !!message.payload.batch_id && message.payload.batch_id === props.batch?.batch_id

        return (
          <div key={message.id} className="flex animate-enter flex-col items-start gap-2">
            {message.messageType === 'storyboard' && !items.length ? (
              <StoryboardMessage
                message={message}
                originalText={originalTextBefore(index)}
                disabled={props.sending}
                onStart={(panels) => props.onStart(message, panels)}
                onResplit={props.onResplit}
              />
            ) : items.length ? (
              <div className="w-full">
                <ImageGridMessage
                  items={items}
                  batchId={message.payload.batch_id}
                  status={message.payload.status}
                  polling={props.polling}
                  onDownload={props.onDownload}
                  onSave={props.onSave}
                  onSaveAll={props.onSaveAll}
                  onRegenerate={props.onRegenerate}
                  onCancel={props.onCancel}
                />
              </div>
            ) : (
              <div className="max-w-[85%] rounded-card rounded-bl-md bg-paper px-4 py-2.5 text-sm leading-relaxed text-ink shadow-card ring-1 ring-hairline">
                {message.content || '…'}
              </div>
            )}
            {batchActive && props.polling ? (
              <span className="flex items-center gap-2 text-xs text-ink3">
                <Spin size="small" />
                正在同步生成进度
              </span>
            ) : null}
          </div>
        )
      })}
      <div ref={endRef} />
    </div>
  )
}

export default MessageList
