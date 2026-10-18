import { useCallback, useEffect, useRef, useState } from 'react'
import { App as AntdApp } from 'antd'
import { useNavigate, useSearchParams } from 'react-router-dom'

import Composer from '../../components/chat/Composer'
import ConversationDrawer from '../../components/chat/ConversationDrawer'
import ModelManagerModal from '../../components/chat/ModelManagerModal'
import SaveBatchModal from '../../components/chat/SaveBatchModal'
import StudioConversation from '../../components/chat/StudioConversation'
import SaveToBookModal from '../../components/ai/SaveToBookModal'
import AppHeader from '../../components/layout/AppHeader'
import Wordmark from '../../components/layout/Wordmark'
import { hasCachedChatSession, useChatGeneration } from '../../hooks/useChatGeneration'
import { aiCredentialsApi } from '../../api/aiCredentials'
import { imagesApi } from '../../api/images'
import { useAuthStore } from '../../stores/authStore'
import type { BatchItem, ChatMessage, StoryboardPanel } from '../../types/conversation'
import { ArtStyle } from '../../types/generation'

/**
 * 统一体验页（搜索式）：未发送时是大 LOGO + 极简对话框，发送首条消息后
 * LOGO 上移渐隐、无缝切入 AI 对话工作室。游客与登录用户首屏一致，仪表盘入口改为顶栏导航。
 */
function Home() {
  const { message } = AntdApp.useApp()
  const navigate = useNavigate()
  const [searchParams] = useSearchParams()
  const bookId = searchParams.get('bookId') ?? undefined
  const isAuthenticated = useAuthStore((state) => state.isAuthenticated)

  // 同一次 SPA 内返回首页时，直接恢复上一次会话（游客也有效）
  const restored = !bookId && hasCachedChatSession()

  const chat = useChatGeneration()
  const [text, setText] = useState('')
  const [style, setStyle] = useState<ArtStyle>(ArtStyle.CARTOON)
  const [size, setSize] = useState('1024x1024')
  const [count, setCount] = useState(1)
  const [seed, setSeed] = useState<number | null>(null)

  // 首页态 → 工作室态 的切换；首条发送成功或恢复历史会话后置 true
  const [started, setStarted] = useState(restored)
  // 大 LOGO 渐隐结束后卸载，避免深层的极简 Composer 仍可被 Tab 聚焦
  const [heroGone, setHeroGone] = useState(restored)
  const [quotaDismissed, setQuotaDismissed] = useState(false)
  const [historyOpen, setHistoryOpen] = useState(false)
  const [saveImageId, setSaveImageId] = useState<string | null>(null)
  const [saveBatchOpen, setSaveBatchOpen] = useState(false)
  const [batchImageIds, setBatchImageIds] = useState<string[]>([])
  // 文本 + 图像两项都使用自定义模型：免全站限额，并允许自定义张数
  const [byokDual, setByokDual] = useState(false)
  const [modelOpen, setModelOpen] = useState(false)

  const prevAuthRef = useRef(isAuthenticated)

  // 登录/登出切换身份时清空上一身份的会话，避免串号
  useEffect(() => {
    if (prevAuthRef.current === isAuthenticated) return
    prevAuthRef.current = isAuthenticated
    chat.newConversation()
    setStarted(false)
    setHeroGone(false)
    setQuotaDismissed(false)
    setText('')
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isAuthenticated])

  // 读取 BYOK 状态：文本与图像双项都配置时才允许自定义张数
  const refreshByok = useCallback(() => {
    if (!isAuthenticated) {
      setByokDual(false)
      return
    }
    void aiCredentialsApi
      .getOverview()
      .then(({ data }) => {
        const overview = data.data
        setByokDual(Boolean(overview?.credentials.llm && overview?.credentials.image))
      })
      .catch((error) => {
        console.error('读取模型配置失败', error)
        setByokDual(false)
      })
  }, [isAuthenticated])

  useEffect(() => {
    refreshByok()
  }, [refreshByok])

  // 从自定义模型切回系统模型后，把超过 6 的张数收敛回上限
  useEffect(() => {
    if (!byokDual && count > 6) setCount(6)
  }, [byokDual, count])

  useEffect(() => {
    if (!isAuthenticated) {
      // 游客：本次 SPA 会话内的对话已由模块缓存恢复，不在此清空
      return
    }
    void chat.loadConversations().then((items) => {
      if (bookId || items.length === 0) return
      void chat.loadConversation(items[0].id).then((detail) => {
        if (!detail) return
        if (detail.conversation.style) setStyle(detail.conversation.style as ArtStyle)
        if (detail.conversation.size) setSize(detail.conversation.size)
        // 刷新/重新进入时，若最近会话有内容则直接回到工作室
        const hasContent = detail.messages.length > 0
        setStarted(hasContent)
        setHeroGone(hasContent)
      })
    })
    // 只在登录态变化时初始化，避免每次渲染重置会话
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isAuthenticated])

  // 首条发送后，等大 LOGO 上移渐隐的过渡结束再卸载首页层
  useEffect(() => {
    if (!started) return
    const timer = window.setTimeout(() => setHeroGone(true), 520)
    return () => window.clearTimeout(timer)
  }, [started])

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
      .then(() => setStarted(true))
      .catch((error) => {
        console.error('创建分镜失败', error)
        message.error(error instanceof Error ? error.message : '创建分镜失败')
        setText(value)
      })
  }

  /** 新对话：清空会话并回到搜索式首页 */
  const handleNew = () => {
    chat.newConversation()
    setStarted(false)
    setHeroGone(false)
    setQuotaDismissed(false)
    setText('')
  }

  const handleStart = (chatMessage: ChatMessage, panels: StoryboardPanel[]) => {
    const batchId = chatMessage.payload.batch_id
    if (!batchId) return
    void chat
      .startBatch(batchId, panels, bookId, seed, style, size)
      .catch((error) => {
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

  const handleSave = (imageId: string) => {
    if (!isAuthenticated) {
      message.info('登录后即可把图片保存到绘本')
      navigate('/login')
      return
    }
    setSaveImageId(imageId)
  }

  // 首张图生成完成后，才向游客展示额度提示（可关闭；已登录用户永不出现）
  const firstImageDone =
    (chat.batch?.items ?? []).some((item) => item.status === 'completed') ||
    chat.messages.some((m) => (m.payload.images ?? []).some((img) => img.status === 'completed'))
  const showQuotaBanner = !isAuthenticated && firstImageDone && !quotaDismissed

  // 首页态与工作室态共用同一套 Composer 绑定，避免两处重复
  const composerProps = {
    value: text,
    onValueChange: setText,
    style,
    onStyleChange: setStyle,
    size,
    onSizeChange: setSize,
    count,
    onCountChange: setCount,
    seed,
    onSeedChange: setSeed,
    isGuest: !isAuthenticated,
    sending: chat.submitting,
    onSend: handleSend,
    onHistory: () => {
      setHistoryOpen(true)
      if (isAuthenticated) void chat.loadConversations()
    },
    onNew: handleNew,
    byokDual,
    onBooks: () => navigate('/books'),
    onModelManager: () => setModelOpen(true),
  }

  return (
    <div className="relative min-h-screen bg-canvas sc-canvas-wash">
      <AppHeader transparent showLogo={started} />

      {/* 首页态：居中大 LOGO + 极简对话框；发送后上移渐隐，过渡结束卸载 */}
      {!heroGone ? (
        <section
          aria-hidden={started}
          className={`absolute inset-0 z-10 flex flex-col items-center justify-center px-4 transition-all duration-500 ease-out ${
            started ? 'pointer-events-none -translate-y-24 opacity-0' : 'opacity-100'
          }`}
        >
          <Wordmark size="hero" className="mb-10" />
          <div className="w-full max-w-2xl animate-enter">
            <Composer {...composerProps} minimal />
          </div>
        </section>
      ) : null}

      {/* 工作室态：对话区占满 + 底部 Composer */}
      {started ? (
        <StudioConversation
          messages={chat.messages}
          batch={chat.batch}
          polling={chat.polling}
          sending={chat.submitting}
          composerProps={composerProps}
          showQuotaBanner={showQuotaBanner}
          onDismissQuota={() => setQuotaDismissed(true)}
          onLogin={() => navigate('/login')}
          onStart={handleStart}
          onResplit={handleResplit}
          onDownload={handleDownload}
          onSave={handleSave}
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
      ) : null}

      <ConversationDrawer
        open={historyOpen}
        loading={chat.historyLoading}
        conversations={chat.conversations}
        currentId={chat.conversationId}
        onClose={() => setHistoryOpen(false)}
        onSelect={(id) => {
          setHistoryOpen(false)
          void chat.loadConversation(id).then((detail) => {
            if (!detail) return
            if (detail.conversation.style) setStyle(detail.conversation.style as ArtStyle)
            if (detail.conversation.size) setSize(detail.conversation.size)
            setStarted(true)
            setHeroGone(true)
          })
        }}
        onNew={() => {
          setHistoryOpen(false)
          handleNew()
        }}
        onDelete={(id) => {
          void chat.removeConversation(id).catch(() => message.error('删除会话失败'))
        }}
      />

      <ModelManagerModal
        open={modelOpen}
        onClose={() => setModelOpen(false)}
        onChanged={refreshByok}
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
    </div>
  )
}

export default Home
