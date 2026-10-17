import { useEffect, useState } from 'react'
import { Link, useNavigate, useSearchParams } from 'react-router-dom'
import { Alert, App, Button, Space, Spin, Tag, Typography } from 'antd'
import { BulbOutlined, HistoryOutlined, PlusOutlined } from '@ant-design/icons'

import Composer from '../../components/chat/Composer'
import ConversationDrawer from '../../components/chat/ConversationDrawer'
import MessageList from '../../components/chat/MessageList'
import SaveBatchModal from '../../components/chat/SaveBatchModal'
import SaveToBookModal from '../../components/ai/SaveToBookModal'
import AppLayout from '../../components/layout/AppLayout'
import { useChatGeneration } from '../../hooks/useChatGeneration'
import { imagesApi } from '../../api/images'
import { useAuthStore } from '../../stores/authStore'
import type { BatchItem, ChatMessage, StoryboardPanel } from '../../types/conversation'
import { ArtStyle } from '../../types/generation'

const { Text, Paragraph } = Typography

const PROMPTS = [
  '小女孩在雨后的森林里发现一只受伤的小鸟，她把它捧在手心带回家。',
  '小熊第一次独自坐火车去海边，路上遇到一只会唱歌的鲸鱼。',
  '夜晚的屋顶上，小猫和月亮交换了一个秘密。',
]

/**
 * 对话式创作页（替代原表单式 Generate）。
 *
 * 流程：发送故事 → AI 分镜 → 逐格确认 → 批次出图 → 单图/整组入绘本。
 * `/generate?bookId=xxx` 仍保留，图片可直接落入当前绘本。
 */
function Generate() {
  const { message } = App.useApp()
  const navigate = useNavigate()
  const [searchParams] = useSearchParams()
  const bookId = searchParams.get('bookId') ?? undefined
  const isAuthenticated = useAuthStore((state) => state.isAuthenticated)

  const chat = useChatGeneration()
  const [text, setText] = useState('')
  const [style, setStyle] = useState<ArtStyle>(ArtStyle.CARTOON)
  const [size, setSize] = useState('1024x1024')
  const [count, setCount] = useState(1)
  const [seed, setSeed] = useState<number | null>(null)
  const [historyOpen, setHistoryOpen] = useState(false)
  const [saveImageId, setSaveImageId] = useState<string | null>(null)
  const [saveBatchOpen, setSaveBatchOpen] = useState(false)
  const [batchImageIds, setBatchImageIds] = useState<string[]>([])

  useEffect(() => {
    if (!isAuthenticated) {
      chat.newConversation()
      return
    }
    void chat.loadConversations().then((items) => {
      if (!bookId && items.length > 0) {
        void chat.loadConversation(items[0].id).then((conversation) => {
          if (conversation?.style) setStyle(conversation.style as ArtStyle)
          if (conversation?.size) setSize(conversation.size)
        })
      }
    })
    // 只在登录态变化时初始化，避免每次渲染重置会话
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isAuthenticated])

  const handleSend = () => {
    const value = text.trim()
    if (!value || chat.submitting) return
    setText('')
    void chat
      .sendPlan({
        text: value,
        imageCount: isAuthenticated ? count : 1,
        style,
        size,
        bookId,
        seed,
      })
      .catch((error) => {
        console.error('创建分镜失败', error)
        message.error(error instanceof Error ? error.message : '创建分镜失败')
        setText(value)
      })
  }

  const handleStart = (chatMessage: ChatMessage, panels: StoryboardPanel[]) => {
    const batchId = chatMessage.payload.batch_id
    if (!batchId) return
    void chat.startBatch(batchId, panels, bookId, seed, style, size).catch((error) => {
      console.error('启动生成失败', error)
      message.error(error instanceof Error ? error.message : '启动生成失败')
    })
  }

  const handleResplit = (value: string) => {
    void chat
      .sendPlan({
        text: value,
        imageCount: isAuthenticated ? count : 1,
        style,
        size,
        bookId,
        seed,
      })
      .catch((error) => {
        console.error('重新拆分失败', error)
        message.error('重新拆分失败，请稍后重试')
      })
  }

  const handleDownload = (imageId: string) => {
    void imagesApi.download(imageId).catch((error) => {
      console.error('下载图片失败', error)
      message.error('下载图片失败')
    })
  }

  const handleSaveAll = (items: BatchItem[]) => {
    if (!isAuthenticated) {
      message.info('登录后即可把图片保存到绘本')
      navigate('/login')
      return
    }
    const imageIds = items
      .filter((item) => item.status === 'completed' && item.image_id)
      .map((item) => item.image_id as string)
    if (imageIds.length === 0) {
      message.info('还没有可加入绘本的图片')
      return
    }
    setBatchImageIds(imageIds)
    setSaveBatchOpen(true)
  }

  const empty = chat.messages.length === 0

  return (
    <AppLayout variant="app">
      <div className="mx-auto flex h-[calc(100vh-10rem)] w-full max-w-4xl flex-col">
        <div className="mb-3 flex flex-wrap items-start justify-between gap-2">
          <div>
            <h1 className="font-display text-xl font-semibold text-ink">创作对话</h1>
            <Paragraph type="secondary" className="!mb-0 mt-1 text-xs">
              先拆分镜，确认后再出图；一组图共享角色设定、风格与随机种子。
              {bookId ? ' 当前作品将保存到正在编辑的绘本。' : ''}
            </Paragraph>
          </div>
          <div className="flex flex-wrap items-center gap-2">
            {!isAuthenticated ? (
              <Tag color="gold">游客 · 单次 1 张</Tag>
            ) : (
              <Tag color="success">最多 6 张 · 会话自动保存</Tag>
            )}
            <Button
              size="small"
              icon={<HistoryOutlined />}
              onClick={() => {
                setHistoryOpen(true)
                if (isAuthenticated) void chat.loadConversations()
              }}
            >
              历史
            </Button>
            <Button size="small" icon={<PlusOutlined />} onClick={chat.newConversation}>
              新对话
            </Button>
          </div>
        </div>

        {!isAuthenticated ? (
          <Alert
            className="mb-3"
            type="info"
            showIcon
            message={
              <span>
                游客可免费生成 1 张；<Link to="/login">登录后</Link>
                可一次生成最多 6 张，并保存创作会话。
              </span>
            }
          />
        ) : null}

        <div className="min-h-0 flex-1 overflow-y-auto pr-1">
          {empty ? (
            <div className="flex h-full flex-col items-center justify-center gap-4 py-10 text-center">
              <div className="rounded-full bg-canvas2 p-4 text-clay">
                <BulbOutlined className="text-2xl" />
              </div>
              <div>
                <Text strong className="text-base">
                  把一个故事，聊成一本连环画
                </Text>
                <Paragraph type="secondary" className="!mb-0 mt-1 text-sm">
                  告诉我故事，我会先拆成画面，确认后再一张张画出来。
                </Paragraph>
              </div>
              {chat.submitting ? (
                <Space className="text-sm text-ink2">
                  <Spin size="small" />
                  正在理解故事并拆分镜…
                </Space>
              ) : null}
              <div className="flex w-full max-w-xl flex-col gap-2">
                {PROMPTS.map((prompt) => (
                  <button
                    key={prompt}
                    type="button"
                    onClick={() => setText(prompt)}
                    className="rounded-chip border border-hairline bg-paper px-4 py-3 text-left text-sm text-ink2 transition-colors hover:border-clay/40 hover:text-ink"
                  >
                    {prompt}
                  </button>
                ))}
              </div>
            </div>
          ) : (
            <MessageList
              messages={chat.messages}
              batch={chat.batch}
              polling={chat.polling}
              sending={chat.submitting}
              onStart={handleStart}
              onResplit={handleResplit}
              onDownload={handleDownload}
              onSave={(imageId) => {
                if (!isAuthenticated) {
                  message.info('登录后即可把图片保存到绘本')
                  navigate('/login')
                  return
                }
                setSaveImageId(imageId)
              }}
              onSaveAll={handleSaveAll}
              onRegenerate={(index, batchId) => {
                void chat.regeneratePanel(index, batchId).catch((error) => {
                  console.error('单格重画失败', error)
                  message.error('重画失败，请稍后重试')
                })
              }}
              onCancel={(batchId) => {
                void chat.cancelBatch(batchId).catch((error) => {
                  console.error('取消批次失败', error)
                  message.error('取消失败，请稍后重试')
                })
              }}
            />
          )}
        </div>

        <div className="sticky bottom-0 z-10 bg-canvas pt-3">
          <Composer
            value={text}
            onValueChange={setText}
            style={style}
            onStyleChange={setStyle}
            size={size}
            onSizeChange={setSize}
            count={count}
            onCountChange={setCount}
            seed={seed}
            onSeedChange={setSeed}
            isGuest={!isAuthenticated}
            sending={chat.submitting}
            onSend={handleSend}
            onHistory={() => {
              setHistoryOpen(true)
              if (isAuthenticated) void chat.loadConversations()
            }}
            onNew={chat.newConversation}
          />
        </div>
      </div>

      <ConversationDrawer
        open={historyOpen}
        loading={chat.historyLoading}
        conversations={chat.conversations}
        currentId={chat.conversationId}
        onClose={() => setHistoryOpen(false)}
        onSelect={(id) => {
          setHistoryOpen(false)
          void chat.loadConversation(id).then((conversation) => {
            if (conversation?.style) setStyle(conversation.style as ArtStyle)
            if (conversation?.size) setSize(conversation.size)
          })
        }}
        onNew={() => {
          setHistoryOpen(false)
          chat.newConversation()
        }}
        onDelete={(id) => {
          void chat.removeConversation(id).catch(() => message.error('删除会话失败'))
        }}
      />

      <SaveToBookModal
        open={!!saveImageId}
        imageId={saveImageId}
        defaultBookId={bookId}
        onCancel={() => setSaveImageId(null)}
      />

      <SaveBatchModal
        open={saveBatchOpen}
        imageIds={batchImageIds}
        defaultBookId={bookId}
        onCancel={() => setSaveBatchOpen(false)}
      />
    </AppLayout>
  )
}

export default Generate
