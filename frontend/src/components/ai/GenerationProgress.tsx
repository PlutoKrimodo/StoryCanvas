import { useEffect, useRef, useState } from 'react'
import { Alert, Button, Collapse, Progress, Space, Steps, Typography } from 'antd'

import { TaskStatus, type TaskResult } from '../../types/generation'
import { TASK_STATUS_TEXT } from '../../utils/constants'

const { Paragraph, Text } = Typography

interface GenerationProgressProps {
  task: TaskResult | null
  polling: boolean
  onCancel: () => void
  onRegenerate?: () => void
}

/** 阶段条文案：后端只回 pending/processing/completed，这里按生成流水线做静态映射 */
const STAGE_ITEMS = [
  { title: '解析故事' },
  { title: '优化提示词' },
  { title: '绘制插画' },
  { title: '完成' },
]

/** 缓动时间常数（秒）：25s 到 ~58%，60s 到 ~90%，永远不到 100，避免「假完成」 */
const EASE_TAU = 25

function isActive(status: TaskStatus): boolean {
  return status === TaskStatus.PENDING || status === TaskStatus.PROCESSING
}

function stageOf(status: TaskStatus, polling: boolean) {
  switch (status) {
    case TaskStatus.COMPLETED:
      return { current: STAGE_ITEMS.length, status: 'finish' as const }
    case TaskStatus.FAILED:
      return { current: 2, status: 'error' as const }
    case TaskStatus.CANCELLED:
      return { current: 2, status: 'wait' as const }
    default:
      return {
        current: status === TaskStatus.PROCESSING ? 2 : 0,
        status: polling ? ('process' as const) : ('wait' as const),
      }
  }
}

function dotClassOf(status: TaskStatus): string {
  switch (status) {
    case TaskStatus.COMPLETED:
      return 'bg-accent-leaf'
    case TaskStatus.FAILED:
      return 'bg-accent-poppy'
    case TaskStatus.CANCELLED:
      return 'bg-ink3'
    default:
      return 'bg-amber'
  }
}

interface EasedBarProps {
  createdAt: string
  status: TaskStatus
  polling: boolean
}

/**
 * 时间缓动进度条。
 *
 * 单独成组件的原因：主组件在 `if (!task) return null` 之后才拿到 task，
 * 把计时 hook 写在主组件顶部会造成条件调用（docs/17 §4.4）。
 */
function EasedBar({ createdAt, status, polling }: EasedBarProps) {
  const active = isActive(status)
  const fallbackStart = useRef(Date.now())
  const [now, setNow] = useState(() => Date.now())

  useEffect(() => {
    if (!active) {
      return
    }
    fallbackStart.current = Date.now()
    setNow(Date.now())
    const timer = setInterval(() => setNow(Date.now()), 500)
    return () => clearInterval(timer)
  }, [active, createdAt])

  const created = Date.parse(createdAt)
  // created_at 由服务端生成，与本机时钟可能不同步：异常值或明显的未来时间退回本地起点
  const base = Number.isFinite(created) && created <= now + 5000 ? created : fallbackStart.current
  const elapsed = Math.max(0, (now - base) / 1000)
  const percent = active ? 92 * (1 - Math.exp(-elapsed / EASE_TAU)) : 100

  return (
    <div>
      <Progress
        percent={active ? Math.max(3, Math.round(percent)) : Math.round(percent)}
        status={
          status === TaskStatus.FAILED
            ? 'exception'
            : status === TaskStatus.COMPLETED
              ? 'success'
              : active && polling
                ? 'active'
                : 'normal'
        }
      />
      {active ? (
        <Text type="secondary" className="text-xs">
          {polling ? `已用 ${Math.floor(elapsed)}s · 通常 20–40s` : '状态同步已暂停，可稍后重新开始'}
        </Text>
      ) : null}
    </div>
  )
}

/** 生成进度：阶段条 + 缓动进度 + 错误信息与取消入口。 */
function GenerationProgress({ task, polling, onCancel, onRegenerate }: GenerationProgressProps) {
  if (!task) {
    return null
  }

  const active = isActive(task.status)
  const stage = stageOf(task.status, polling)

  return (
    <section className="rounded-card bg-canvas2 px-5 py-4 ring-1 ring-hairline">
      <div className="flex flex-wrap items-center justify-between gap-3">
        <span className="flex items-center gap-2">
          <span className={`h-2 w-2 rounded-full ${dotClassOf(task.status)}`} />
          <Text strong>{TASK_STATUS_TEXT[task.status] ?? task.status}</Text>
        </span>
        <Space size="small">
          {active ? (
            <Button size="small" danger onClick={onCancel}>
              取消生成
            </Button>
          ) : null}
          {!active && onRegenerate ? (
            <Button size="small" onClick={onRegenerate}>
              重新生成
            </Button>
          ) : null}
        </Space>
      </div>

      <div className="mt-4">
        <Steps size="small" current={stage.current} status={stage.status} items={STAGE_ITEMS} />
      </div>

      <div className="mt-4">
        <EasedBar createdAt={task.created_at} status={task.status} polling={polling} />
      </div>

      {task.status === TaskStatus.FAILED && task.error_message ? (
        <Alert className="mt-4" type="error" showIcon message={task.error_message} />
      ) : null}

      {task.prompt ? (
        <div className="mt-4">
          <Text type="secondary" className="block text-xs leading-relaxed line-clamp-2">
            绘图提示词：{task.prompt}
          </Text>
          <Collapse
            ghost
            size="small"
            items={[
              {
                key: 'prompt',
                label: '查看提示词',
                children: (
                  <Paragraph className="!mb-0 text-xs leading-relaxed break-all">{task.prompt}</Paragraph>
                ),
              },
            ]}
          />
        </div>
      ) : null}
    </section>
  )
}

export default GenerationProgress
