import { useEffect } from 'react'
import { Button, Card, Empty, Image, List, Tag, Typography } from 'antd'
import { HistoryOutlined, RedoOutlined } from '@ant-design/icons'

import { useGenerationStore } from '../../stores/generationStore'
import type { TaskResult } from '../../types/generation'
import { TASK_STATUS_COLOR, TASK_STATUS_TEXT } from '../../utils/constants'
import { formatDateTime } from '../../utils/datetime'

const { Text } = Typography

interface GenerationHistoryProps {
  /** 传入后仅展示该绘本下的生成记录 */
  bookId?: string
  /** 复用某次生成的文本与风格（Phase 6：历史记录复用） */
  onReuse: (task: TaskResult) => void
}

/** 生成历史列表：可查看缩略图并把参数回填到生成表单。 */
function GenerationHistory({ bookId, onReuse }: GenerationHistoryProps) {
  const history = useGenerationStore((state) => state.history)
  const loading = useGenerationStore((state) => state.historyLoading)
  const fetchHistory = useGenerationStore((state) => state.fetchHistory)

  useEffect(() => {
    void fetchHistory({ book_id: bookId })
  }, [fetchHistory, bookId])

  return (
    <Card
      title={
        <span>
          <HistoryOutlined className="mr-2" />
          生成历史
        </span>
      }
      className="!rounded-2xl"
    >
      {history.length === 0 && !loading ? (
        <Empty description="还没有生成记录" />
      ) : (
        <List
          loading={loading}
          dataSource={history}
          itemLayout="horizontal"
          renderItem={(task) => (
            <List.Item
              key={task.task_id}
              actions={[
                <Button
                  key="reuse"
                  type="link"
                  size="small"
                  icon={<RedoOutlined />}
                  onClick={() => onReuse(task)}
                >
                  复用参数
                </Button>,
              ]}
            >
              <List.Item.Meta
                avatar={
                  task.result?.image_url ? (
                    <Image
                      src={task.result.image_url}
                      width={64}
                      height={64}
                      className="rounded-lg object-cover"
                      preview={{ src: task.result.image_url }}
                    />
                  ) : (
                    <div className="w-16 h-16 rounded-lg bg-slate-100 flex items-center justify-center text-slate-400 text-xs">
                      无图
                    </div>
                  )
                }
                title={
                  <div className="flex items-center gap-2">
                    <Tag color={TASK_STATUS_COLOR[task.status]}>
                      {TASK_STATUS_TEXT[task.status] ?? task.status}
                    </Tag>
                    <Text type="secondary" className="text-xs">
                      {formatDateTime(task.created_at, 'YYYY-MM-DD HH:mm')}
                    </Text>
                  </div>
                }
                description={
                  <Text type="secondary" className="text-xs line-clamp-2">
                    {task.original_text || task.prompt || '（无文本）'}
                  </Text>
                }
              />
            </List.Item>
          )}
        />
      )}
    </Card>
  )
}

export default GenerationHistory
