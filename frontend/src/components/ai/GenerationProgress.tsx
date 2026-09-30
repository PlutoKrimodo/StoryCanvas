import { Alert, Button, Card, Progress, Space, Tag, Typography } from 'antd'
import { CheckCircleOutlined, CloseCircleOutlined, LoadingOutlined } from '@ant-design/icons'

import { TaskStatus, type TaskResult } from '../../types/generation'
import { TASK_STATUS_COLOR, TASK_STATUS_TEXT } from '../../utils/constants'

const { Text } = Typography

interface GenerationProgressProps {
  task: TaskResult | null
  polling: boolean
  onCancel: () => void
  onRegenerate?: () => void
}

function statusIcon(status?: TaskStatus) {
  switch (status) {
    case TaskStatus.COMPLETED:
      return <CheckCircleOutlined />
    case TaskStatus.FAILED:
      return <CloseCircleOutlined />
    default:
      return <LoadingOutlined />
  }
}

function percentOf(status?: TaskStatus): number {
  switch (status) {
    case TaskStatus.PENDING:
      return 15
    case TaskStatus.PROCESSING:
      return 60
    case TaskStatus.COMPLETED:
    case TaskStatus.FAILED:
    case TaskStatus.CANCELLED:
      return 100
    default:
      return 0
  }
}

/** 生成进度：展示任务状态、进度条、错误信息与取消入口。 */
function GenerationProgress({ task, polling, onCancel, onRegenerate }: GenerationProgressProps) {
  if (!task) {
    return null
  }

  const active = task.status === TaskStatus.PENDING || task.status === TaskStatus.PROCESSING

  return (
    <Card title="生成进度" className="!rounded-2xl">
      <Space direction="vertical" className="w-full" size="middle">
        <div className="flex items-center justify-between">
          <Tag icon={statusIcon(task.status)} color={TASK_STATUS_COLOR[task.status]}>
            {TASK_STATUS_TEXT[task.status] ?? task.status}
          </Tag>
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
        </div>

        <Progress
          percent={percentOf(task.status)}
          status={
            task.status === TaskStatus.FAILED
              ? 'exception'
              : polling
                ? 'active'
                : 'normal'
          }
        />

        {task.status === TaskStatus.FAILED && task.error_message ? (
          <Alert type="error" showIcon message={task.error_message} />
        ) : null}

        {task.prompt ? (
          <Text type="secondary" className="text-xs break-all">
            绘图提示词：{task.prompt}
          </Text>
        ) : null}
      </Space>
    </Card>
  )
}

export default GenerationProgress
