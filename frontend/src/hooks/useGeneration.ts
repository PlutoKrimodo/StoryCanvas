import { useCallback, useEffect, useRef, useState } from 'react'

import { generationApi } from '../api/generation'
import { useGenerationStore } from '../stores/generationStore'
import {
  TaskStatus,
  type GenerationRequest,
  type RegenerateRequest,
  type TaskResult,
} from '../types/generation'

/** 轮询间隔与总超时（与 docs/16 §5.4 「AI 生成 < 60s」留足余量） */
const POLL_INTERVAL = 2000
const MAX_POLL_MS = 5 * 60 * 1000

interface UseGenerationResult {
  currentTask: TaskResult | null
  polling: boolean
  createTask: (payload: GenerationRequest) => Promise<TaskResult | null>
  regenerate: (taskId: string, payload?: RegenerateRequest) => Promise<TaskResult | null>
  cancel: (taskId: string) => Promise<boolean>
  reset: () => void
}

/**
 * 生成任务 Hook：负责创建 / 重新生成 / 取消，并用轮询跟踪任务状态。
 *
 * 设计要点：轮询逻辑收敛在此处（而不是各组件各写一份），
 * 组件只需渲染 `currentTask`。
 */
export function useGeneration(): UseGenerationResult {
  const currentTask = useGenerationStore((state) => state.currentTask)
  const polling = useGenerationStore((state) => state.polling)
  const setCurrentTask = useGenerationStore((state) => state.setCurrentTask)
  const setPolling = useGenerationStore((state) => state.setPolling)
  const resetStore = useGenerationStore((state) => state.reset)

  // 被跟踪的任务 ID：变化时触发轮询 effect
  const [trackedTaskId, setTrackedTaskId] = useState<string | null>(null)
  const pollingRef = useRef(false)

  useEffect(() => {
    if (!trackedTaskId) {
      return
    }

    let disposed = false
    let timer: ReturnType<typeof setTimeout> | null = null
    const startedAt = Date.now()
    pollingRef.current = true
    setPolling(true)

    const tick = async () => {
      try {
        const { data } = await generationApi.getTask(trackedTaskId)
        if (disposed) {
          return
        }
        const task = data.data
        if (task) {
          setCurrentTask(task)
          if (
            task.status === TaskStatus.COMPLETED ||
            task.status === TaskStatus.FAILED ||
            task.status === TaskStatus.CANCELLED
          ) {
            pollingRef.current = false
            setPolling(false)
            return
          }
        }
      } catch (error) {
        // 瞬时网络错误不中断轮询；持续失败由总超时兜底
        console.error('轮询任务状态失败', error)
        if (disposed) {
          return
        }
      }

      if (Date.now() - startedAt > MAX_POLL_MS) {
        pollingRef.current = false
        setPolling(false)
        return
      }
      timer = setTimeout(tick, POLL_INTERVAL)
    }

    void tick()

    return () => {
      disposed = true
      pollingRef.current = false
      if (timer) {
        clearTimeout(timer)
      }
    }
  }, [trackedTaskId, setCurrentTask, setPolling])

  const track = useCallback(
    (task: TaskResult) => {
      setCurrentTask(task)
      setTrackedTaskId(task.task_id)
    },
    [setCurrentTask],
  )

  const createTask = useCallback(
    async (payload: GenerationRequest): Promise<TaskResult | null> => {
      const { data } = await generationApi.createTask(payload)
      if (!data.data) {
        throw new Error(data.message || '创建生成任务失败')
      }
      const initial: TaskResult = {
        task_id: data.data.task_id,
        status: data.data.status,
        prompt: '',
        original_text: payload.text,
        created_at: data.data.created_at,
      }
      track(initial)
      return initial
    },
    [track],
  )

  const regenerate = useCallback(
    async (taskId: string, payload?: RegenerateRequest): Promise<TaskResult | null> => {
      const { data } = await generationApi.regenerate(taskId, payload)
      if (!data.data) {
        throw new Error(data.message || '重新生成失败')
      }
      const initial: TaskResult = {
        task_id: data.data.task_id,
        status: data.data.status,
        prompt: '',
        created_at: data.data.created_at,
      }
      track(initial)
      return initial
    },
    [track],
  )

  const cancel = useCallback(async (taskId: string): Promise<boolean> => {
    try {
      await generationApi.cancel(taskId)
      return true
    } catch (error) {
      console.error('取消任务失败', error)
      return false
    }
  }, [])

  const reset = useCallback(() => {
    setTrackedTaskId(null)
    resetStore()
  }, [resetStore])

  return { currentTask, polling, createTask, regenerate, cancel, reset }
}
