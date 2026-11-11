import { useState } from 'react'
import { Alert, Button, Empty, Space, Spin, Tag, Typography } from 'antd'
import {
  DownloadOutlined,
  PlusOutlined,
  ReloadOutlined,
  SaveOutlined,
  StopOutlined,
} from '@ant-design/icons'

import ArtImage from '../common/ArtImage'
import ImageLightbox from './ImageLightbox'
import type { BatchItem } from '../../types/conversation'
import { formatTaskError, isContentRejected } from '../../utils/constants'

const { Text } = Typography

interface ImageGridMessageProps {
  items: BatchItem[]
  batchId?: string
  status?: string
  polling: boolean
  onDownload: (imageId: string) => void
  onSave: (imageId: string) => void
  onSaveAll: (items: BatchItem[]) => void
  onRegenerate: (panelIndex: number, batchId?: string) => void
  onCancel: (batchId?: string) => void
}

const ACTIVE = new Set(['pending', 'processing'])

/** 批次图片网格：逐张状态、单图操作与整组操作。 */
function ImageGridMessage({
  items,
  batchId,
  status,
  polling,
  onDownload,
  onSave,
  onSaveAll,
  onRegenerate,
  onCancel,
}: ImageGridMessageProps) {
  const [preview, setPreview] = useState<{ src: string; alt: string } | null>(null)

  const completed = items.filter((item) => item.status === 'completed' && item.image_id)
  const active = ACTIVE.has(status ?? '')

  const openPreview = (src: string, alt: string) => setPreview({ src, alt })

  if (!items.length) {
    return (
      <div className="rounded-card border border-hairline bg-canvas2/60 p-4">
        <Space>
          <Spin size="small" />
          <Text type="secondary">正在准备画面…</Text>
        </Space>
      </div>
    )
  }

  return (
    <div className="rounded-card border border-hairline bg-canvas2/60 p-4">
      <div className="mb-3 flex flex-wrap items-center justify-between gap-2">
        <Space wrap>
          <Text strong>本组 {items.length} 张</Text>
          {status ? <Tag>{status}</Tag> : null}
        </Space>
        <Space wrap>
          {completed.length ? (
            <Button size="small" icon={<SaveOutlined />} onClick={() => onSaveAll(items)}>
              全部加入绘本
            </Button>
          ) : null}
          {active ? (
            <Button size="small" danger icon={<StopOutlined />} onClick={() => onCancel(batchId)}>
              取消剩余
            </Button>
          ) : null}
        </Space>
      </div>

      <div className="grid gap-3 sm:grid-cols-2">
        {items.map((item) => {
          const index = item.panel_index ?? 0
          const running = item.status === 'pending' || item.status === 'processing'
          const hasImage = Boolean(item.image_url && item.image_id)
          const contentRejected = isContentRejected(item.error_code)
          const alt = `第 ${index + 1} 格`

          return (
            <div
              key={`${item.task_id}-${index}`}
              className="overflow-hidden rounded-chip border border-hairline bg-paper"
            >
              <div
                className={`relative min-h-[180px] ${hasImage ? 'cursor-zoom-in' : ''}`}
                onDoubleClick={() => {
                  if (hasImage && item.image_url) openPreview(item.image_url, alt)
                }}
              >
                {hasImage && item.image_url ? (
                  <ArtImage
                    src={item.image_url}
                    alt={alt}
                    wrapperClassName="rounded-none"
                    maxHeight={320}
                  />
                ) : (
                  <div className="flex min-h-[180px] items-center justify-center bg-canvas2">
                    {running ? (
                      <Space direction="vertical" align="center" size={6}>
                        <Spin />
                        <Text type="secondary" className="text-xs">
                          {polling ? '绘制中…' : '等待同步'}
                        </Text>
                      </Space>
                    ) : (
                      <Empty image={Empty.PRESENTED_IMAGE_SIMPLE} description="本格未生成" />
                    )}
                  </div>
                )}

                <Tag className="!absolute left-2 top-2" color="processing">
                  第 {index + 1} 格
                </Tag>

                {hasImage && item.image_url ? (
                  <button
                    type="button"
                    aria-label="放大查看"
                    onClick={(event) => {
                      event.stopPropagation()
                      if (item.image_url) openPreview(item.image_url, alt)
                    }}
                    onDoubleClick={(event) => event.stopPropagation()}
                    className="absolute right-2 top-2 z-10 flex h-8 w-8 items-center justify-center rounded-full bg-paper/90 text-ink shadow-card ring-1 ring-hairline transition-transform duration-200 hover:scale-110 hover:bg-paper focus:outline-none focus-visible:ring-2 focus-visible:ring-clay"
                  >
                    <PlusOutlined />
                  </button>
                ) : null}
              </div>

              <div className="p-2">
                {item.status === 'failed' && item.error_message ? (
                  <Alert
                    type="error"
                    showIcon
                    className="mb-2"
                    message={formatTaskError(item.error_message)}
                    description={
                      contentRejected
                        ? '请修改该格描述后重新创建分镜；该失败不会自动重试。'
                        : undefined
                    }
                  />
                ) : null}
                <div className="flex flex-wrap items-center gap-1">
                  <Button
                    size="small"
                    icon={<DownloadOutlined />}
                    disabled={!item.image_id}
                    onClick={() => item.image_id && onDownload(item.image_id)}
                  >
                    下载
                  </Button>
                  <Button
                    size="small"
                    icon={<SaveOutlined />}
                    disabled={!item.image_id}
                    onClick={() => item.image_id && onSave(item.image_id)}
                  >
                    加入绘本
                  </Button>
                  {!contentRejected ? (
                    <Button
                      size="small"
                      type="link"
                      icon={<ReloadOutlined />}
                      disabled={running}
                      onClick={() => onRegenerate(index, batchId)}
                    >
                      重画
                    </Button>
                  ) : null}
                </div>
              </div>
            </div>
          )
        })}
      </div>

      <ImageLightbox
        open={Boolean(preview)}
        src={preview?.src ?? ''}
        alt={preview?.alt}
        onClose={() => setPreview(null)}
      />
    </div>
  )
}

export default ImageGridMessage
