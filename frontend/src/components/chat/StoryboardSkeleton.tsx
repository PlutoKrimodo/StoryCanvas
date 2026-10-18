import { useEffect, useState } from 'react'
import { Skeleton, Typography } from 'antd'

const { Text } = Typography

const STAGES = [
  '正在理解故事…',
  '正在拆分镜…',
  '正在准备画面…',
  '仍在生成，请稍候…',
]

/** 分镜生成中的占位卡片：阶段文案 + shimmer，缓解 LLM 等待感。 */
function StoryboardSkeleton() {
  const [stage, setStage] = useState(0)

  useEffect(() => {
    const timers = [
      window.setTimeout(() => setStage(1), 4000),
      window.setTimeout(() => setStage(2), 10000),
      window.setTimeout(() => setStage(3), 20000),
    ]
    return () => timers.forEach((timer) => window.clearTimeout(timer))
  }, [])

  return (
    <div
      role="status"
      aria-live="polite"
      className="w-full animate-enter rounded-card border border-hairline bg-canvas2/70 p-4"
    >
      <div className="mb-3 flex flex-wrap items-center gap-2">
        <span className="h-2.5 w-2.5 animate-pulse rounded-full bg-clay" />
        <Text strong>AI 正在创作</Text>
        <Text type="secondary" className="text-xs">
          {STAGES[stage]}
        </Text>
      </div>
      <Skeleton active title={false} paragraph={{ rows: 4 }} />
    </div>
  )
}

export default StoryboardSkeleton
