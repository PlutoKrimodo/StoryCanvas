import { create } from 'zustand'

import { generationApi } from '../api/generation'
import type { GenerationHistoryQuery, TaskResult } from '../types/generation'

interface GenerationState {
  /** 当前正在跟踪的任务（创建 / 重新生成后写入） */
  currentTask: TaskResult | null
  /** 生成历史 */
  history: TaskResult[]
  historyTotal: number
  historyLoading: boolean
  /** 是否正在轮询任务状态 */
  polling: boolean

  setCurrentTask: (task: TaskResult | null) => void
  setPolling: (polling: boolean) => void
  fetchHistory: (params?: GenerationHistoryQuery) => Promise<void>
  reset: () => void
}

/** 生成状态：错误提示统一由 axios 响应拦截器负责。 */
export const useGenerationStore = create<GenerationState>((set) => ({
  currentTask: null,
  history: [],
  historyTotal: 0,
  historyLoading: false,
  polling: false,

  setCurrentTask: (task) => set({ currentTask: task }),
  setPolling: (polling) => set({ polling }),

  fetchHistory: (params) => {
    set({ historyLoading: true })
    return generationApi
      .getHistory({ page: 1, limit: 20, ...params })
      .then(({ data }) => {
        set({
          history: data.data?.items ?? [],
          historyTotal: data.data?.total ?? 0,
        })
      })
      .catch((error) => {
        console.error('加载生成历史失败', error)
      })
      .finally(() => set({ historyLoading: false }))
  },

  reset: () => set({ currentTask: null, polling: false }),
}))
