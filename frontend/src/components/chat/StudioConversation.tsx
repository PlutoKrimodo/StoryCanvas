import { Alert, Button, Typography } from 'antd'

import Composer, { type ComposerProps } from './Composer'
import MessageList from './MessageList'
import type { BatchItem, BatchView, ChatMessage, StoryboardPanel } from '../../types/conversation'

const { Text } = Typography

export interface StudioConversationProps {
  messages: ChatMessage[]
  batch: BatchView | null
  polling: boolean
  sending: boolean
  composerProps: ComposerProps
  showQuotaBanner: boolean
  onDismissQuota: () => void
  onLogin: () => void
  onStart: (message: ChatMessage, panels: StoryboardPanel[]) => void
  onResplit: (text: string) => void
  onDownload: (imageId: string) => void
  onSave: (imageId: string) => void
  onSaveAll: (items: BatchItem[]) => void
  onRegenerate: (panelIndex: number, batchId?: string) => void
  onCancel: (batchId?: string) => void
}

/** 工作室态：对话区占满视口，底部 Composer 常驻。 */
function StudioConversation({
  messages,
  batch,
  polling,
  sending,
  composerProps,
  showQuotaBanner,
  onDismissQuota,
  onLogin,
  onStart,
  onResplit,
  onDownload,
  onSave,
  onSaveAll,
  onRegenerate,
  onCancel,
}: StudioConversationProps) {
  return (
    <main className="relative z-0 mx-auto flex h-[calc(100vh-4rem)] w-full max-w-4xl flex-col px-4 pt-4">
      {showQuotaBanner ? (
        <Alert
          type="info"
          showIcon
          closable
          onClose={onDismissQuota}
          className="mb-3"
          message="游客可免费生成 1 张；登录后可一次生成最多 6 张，并保存创作会话。"
          action={
            <Button size="small" type="primary" onClick={onLogin}>
              登录
            </Button>
          }
        />
      ) : null}

      <div className="min-h-0 flex-1 overflow-y-auto py-3 pr-1">
        {messages.length === 0 ? (
          <div className="flex h-full flex-col items-center justify-center gap-2 text-center">
            <Text type="secondary">正在准备创作画布…</Text>
          </div>
        ) : (
          <MessageList
            messages={messages}
            batch={batch}
            polling={polling}
            sending={sending}
            onStart={onStart}
            onResplit={onResplit}
            onDownload={onDownload}
            onSave={onSave}
            onSaveAll={onSaveAll}
            onRegenerate={onRegenerate}
            onCancel={onCancel}
          />
        )}
      </div>

      <div className="border-t border-hairline bg-canvas pt-3">
        <Composer {...composerProps} paramsVisible />
      </div>
    </main>
  )
}

export default StudioConversation
