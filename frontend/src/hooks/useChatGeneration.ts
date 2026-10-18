import { useCallback, useEffect, useRef, useState } from 'react'

import { conversationsApi } from '../api/conversations'
import type {
  BatchView,
  ChatMessage,
  ConversationDetail,
  ConversationSummary,
  PlanResponse,
  StoryboardPanel,
} from '../types/conversation'
import type { ArtStyle } from '../types/generation'

const POLL_INTERVAL = 2000
const TERMINAL = new Set(['completed', 'partial', 'failed', 'cancelled'])

function nowText() {
  return new Date().toISOString()
}

function toMessage(raw: {
  id: string
  role: 'user' | 'assistant'
  message_type: 'text' | 'storyboard' | 'images'
  content?: string | null
  payload: ChatMessage['payload']
  created_at: string
}): ChatMessage {
  return {
    id: raw.id,
    role: raw.role,
    messageType: raw.message_type,
    content: raw.content,
    payload: raw.payload ?? {},
    createdAt: raw.created_at,
  }
}

/**
 * 会话内存缓存。
 *
 * Home 在路由切换（例如去「我的绘本」再返回）时会被卸载，组件内的 useState
 * 随之丢失。把最近一次会话镜像到模块级变量，可在同一次 SPA 会话内无缝恢复；
 * 整页刷新时再由服务端历史兜底。
 */
interface ChatSessionCache {
  conversationId: string | null
  messages: ChatMessage[]
  batch: BatchView | null
}

let sessionCache: ChatSessionCache = {
  conversationId: null,
  messages: [],
  batch: null,
}

/** 是否存在可恢复的会话（供页面决定首屏是否直接进入工作室） */
export function hasCachedChatSession(): boolean {
  return sessionCache.messages.length > 0
}

/** 清空会话缓存（登出 / 切换身份时调用，避免不同账号串数据） */
export function clearChatSessionCache(): void {
  sessionCache = { conversationId: null, messages: [], batch: null }
}

export interface SendPlanOptions {
  text: string
  imageCount: number
  style: ArtStyle
  size: string
  bookId?: string
  seed?: number | null
  /** 是否先渲染乐观用户消息与分镜占位；重新拆分场景不应传 */
  optimistic?: boolean
}

/**
 * 对话式生成状态机：分镜 → 用户确认 → 批次 → 轮询 → 单格重画。
 *
 * 游客不调用会话接口，消息只保存在本 Hook 的内存中。
 */
export function useChatGeneration() {
  // 从模块缓存初始化：路由切走再回来（同一次 SPA）时无需重新拉取即可恢复
  const [conversationId, setConversationId] = useState<string | null>(sessionCache.conversationId)
  const [messages, setMessages] = useState<ChatMessage[]>(sessionCache.messages)
  const [batch, setBatch] = useState<BatchView | null>(sessionCache.batch)
  const [polling, setPolling] = useState(false)
  const [submitting, setSubmitting] = useState(false)
  const [conversations, setConversations] = useState<ConversationSummary[]>([])
  const [historyLoading, setHistoryLoading] = useState(false)

  // 把最新会话镜像到模块缓存，页面卸载后仍可恢复
  useEffect(() => {
    sessionCache = { conversationId, messages, batch }
  }, [conversationId, messages, batch])

  const pollTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  const pollBatchId = useRef<string | null>(null)

  const stopPolling = useCallback(() => {
    if (pollTimer.current) {
      clearTimeout(pollTimer.current)
      pollTimer.current = null
    }
    pollBatchId.current = null
    setPolling(false)
  }, [])

  const applyBatch = useCallback((view: BatchView) => {
    setBatch(view)
    setMessages((prev) =>
      prev.map((message) =>
        message.payload.batch_id === view.batch_id
          ? {
              ...message,
              payload: {
                ...message.payload,
                status: view.status,
                images: view.items,
                panels: view.panels?.length ? view.panels : message.payload.panels,
                seed: view.seed ?? null,
              },
            }
          : message,
      ),
    )
  }, [])

  const pollOnce = useCallback(
    async (batchId: string) => {
      try {
        const { data } = await conversationsApi.getBatch(batchId)
        const view = data.data
        if (!view) {
          stopPolling()
          return
        }
        applyBatch(view)
        if (TERMINAL.has(view.status)) {
          stopPolling()
          return
        }
      } catch (error) {
        console.error('轮询批次状态失败', error)
      }
      if (pollBatchId.current === batchId) {
        pollTimer.current = setTimeout(() => void pollOnce(batchId), POLL_INTERVAL)
      }
    },
    [applyBatch, stopPolling],
  )

  const startPolling = useCallback(
    (batchId: string) => {
      stopPolling()
      pollBatchId.current = batchId
      setPolling(true)
      pollTimer.current = setTimeout(() => void pollOnce(batchId), POLL_INTERVAL)
    },
    [pollOnce, stopPolling],
  )

  // 路由切回时若缓存中的批次仍在生成，恢复轮询
  useEffect(() => {
    const cached = sessionCache.batch
    if (cached && (cached.status === 'pending' || cached.status === 'processing')) {
      startPolling(cached.batch_id)
    }
    // 只在首次挂载时尝试恢复
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  useEffect(() => stopPolling, [stopPolling])

  const loadConversations = useCallback(async () => {
    setHistoryLoading(true)
    try {
      const { data } = await conversationsApi.list({ page: 1, limit: 30 })
      const items = data.data?.items ?? []
      setConversations(items)
      return items
    } catch (error) {
      console.error('加载会话列表失败', error)
      return []
    } finally {
      setHistoryLoading(false)
    }
  }, [])

  const loadConversation = useCallback(
    async (id: string) => {
      stopPolling()
      try {
        const { data } = await conversationsApi.get(id)
        const detail = data.data
        if (!detail) return
        setConversationId(id)
        setBatch(null)
        setMessages(detail.messages.map(toMessage))

        const last = [...detail.messages]
          .reverse()
          .find((message) => typeof message.payload?.batch_id === 'string')
        const lastBatchId = last?.payload?.batch_id
        if (lastBatchId) {
          const batchRes = await conversationsApi.getBatch(lastBatchId)
          const view = batchRes.data.data
          if (view) {
            applyBatch(view)
            if (view.status === 'pending' || view.status === 'processing') {
              startPolling(view.batch_id)
            }
          }
        }
        return detail
      } catch (error) {
        console.error('加载会话失败', error)
        return undefined
      }
    },
    [applyBatch, startPolling, stopPolling],
  )

  const newConversation = useCallback(() => {
    stopPolling()
    clearChatSessionCache()
    setConversationId(null)
    setMessages([])
    setBatch(null)
  }, [stopPolling])

  const removeConversation = useCallback(
    async (id: string) => {
      await conversationsApi.remove(id)
      setConversations((prev) => prev.filter((item) => item.id !== id))
      if (conversationId === id) newConversation()
    },
    [conversationId, newConversation],
  )

  const startBatch = useCallback(
    async (
      batchId: string,
      panels: StoryboardPanel[],
      bookId?: string,
      seed?: number | null,
      style?: ArtStyle,
      size?: string,
    ) => {
      setSubmitting(true)
      try {
        await conversationsApi.updatePlan(batchId, panels)
        const { data } = await conversationsApi.startBatch(batchId, {
          book_id: bookId,
          seed: typeof seed === 'number' ? seed : undefined,
          style,
          size,
        })
        const view = data.data
        if (!view) throw new Error(data.message || '启动生成失败')
        applyBatch(view)
        startPolling(batchId)
        return view
      } finally {
        setSubmitting(false)
      }
    },
    [applyBatch, startPolling],
  )

  const sendPlan = useCallback(
    async (options: SendPlanOptions) => {
      setSubmitting(true)

      // 乐观占位：点击发送后立即出现用户气泡 + 分镜骨架屏，避免空白等待
      const pendingUserId = options.optimistic ? `local-user-pending-${Date.now()}` : ''
      const pendingAiId = options.optimistic ? `local-ai-pending-${Date.now()}` : ''
      if (options.optimistic) {
        const pendingUser: ChatMessage = {
          id: pendingUserId,
          role: 'user',
          messageType: 'text',
          content: options.text,
          payload: {},
          createdAt: nowText(),
          local: true,
        }
        const pendingAssistant: ChatMessage = {
          id: pendingAiId,
          role: 'assistant',
          messageType: 'text',
          content: '',
          payload: {},
          createdAt: nowText(),
          local: true,
          pending: true,
        }
        setMessages((prev) => [...prev, pendingUser, pendingAssistant])
      }

      const removePending = (prev: ChatMessage[]) =>
        options.optimistic
          ? prev.filter((message) => message.id !== pendingUserId && message.id !== pendingAiId)
          : prev

      try {
        const { data } = await conversationsApi.createPlan({
          conversation_id: conversationId ?? undefined,
          text: options.text,
          image_count: options.imageCount,
          style: options.style,
          size: options.size,
        })
        const plan = data.data
        if (!plan) throw new Error(data.message || '分镜生成失败')

        setConversationId(plan.conversation_id)
        setBatch(null)
        const userMessage: ChatMessage = {
          id: `local-user-${Date.now()}`,
          role: 'user',
          messageType: 'text',
          content: options.text,
          payload: {},
          createdAt: nowText(),
          local: true,
        }
        const assistantMessage: ChatMessage = {
          id: plan.message_id ?? `local-ai-${Date.now()}`,
          role: 'assistant',
          messageType: 'storyboard',
          content: plan.title,
          payload: {
            batch_id: plan.batch_id,
            panels: plan.panels,
            consistency: plan.consistency,
            status: 'draft',
          },
          createdAt: nowText(),
          local: true,
        }
        setMessages((prev) => [...removePending(prev), userMessage, assistantMessage])

        // 单张时减少一次确认点击；多张必须由用户先确认分镜
        if (plan.panel_count === 1) {
          await startBatch(
            plan.batch_id,
            plan.panels,
            options.bookId,
            options.seed,
            options.style,
            options.size,
          )
        }
        return plan as PlanResponse
      } catch (error) {
        if (options.optimistic) {
          setMessages((prev) =>
            prev.filter((message) => message.id !== pendingUserId && message.id !== pendingAiId),
          )
        }
        throw error
      } finally {
        setSubmitting(false)
      }
    },
    [conversationId, startBatch],
  )

  const cancelBatch = useCallback(
    async (batchId?: string) => {
      const id = batchId ?? batch?.batch_id
      if (!id) return
      const { data } = await conversationsApi.cancelBatch(id)
      if (data.data) {
        applyBatch(data.data)
        if (data.data.status === 'pending' || data.data.status === 'processing') {
          // 已发出的图像调用不能强行中断，继续轮询直到剩余任务收敛
          startPolling(id)
        } else if (pollBatchId.current === id) {
          stopPolling()
        }
      }
    },
    [applyBatch, batch, startPolling, stopPolling],
  )

  const regeneratePanel = useCallback(
    async (panelIndex: number, batchId?: string) => {
      const id = batchId ?? batch?.batch_id
      if (!id) return
      const { data } = await conversationsApi.regeneratePanel(id, panelIndex)
      if (data.data) {
        applyBatch(data.data)
        startPolling(data.data.batch_id)
      }
    },
    [applyBatch, batch, startPolling],
  )

  return {
    conversationId,
    messages,
    batch,
    polling,
    submitting,
    conversations,
    historyLoading,
    loadConversations,
    loadConversation,
    newConversation,
    removeConversation,
    sendPlan,
    startBatch,
    cancelBatch,
    regeneratePanel,
  }
}
