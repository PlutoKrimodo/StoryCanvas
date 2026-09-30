import { useState } from 'react'
import { Button, Card, Collapse, Empty, Space, Tag, Typography, message } from 'antd'
import { DownloadOutlined, ReloadOutlined, SaveOutlined } from '@ant-design/icons'

import { imagesApi } from '../../api/images'
import type { TaskResult } from '../../types/generation'

const { Paragraph, Text } = Typography

interface ResultPreviewProps {
  task: TaskResult
  onRegenerate: () => void
  /** 传入该图 ID 以打开「保存到绘本」 */
  onSaveToBook: (imageId: string) => void
}

function renderParsed(task: TaskResult) {
  const parsed = task.parsed_data
  if (!parsed) {
    return <Text type="secondary">暂无结构化解析结果</Text>
  }

  const sceneEntries = Object.entries(parsed.scene ?? {}).filter(([, value]) => !!value)

  return (
    <Space direction="vertical" size={4} className="w-full">
      {parsed.characters?.length ? (
        <div>
          <Text strong>角色：</Text>
          {parsed.characters.map((character) => (
            <Tag key={character.name}>{character.name}</Tag>
          ))}
        </div>
      ) : null}
      {sceneEntries.length ? (
        <div>
          <Text strong>场景：</Text>
          {sceneEntries.map(([key, value]) => (
            <Tag key={key}>{`${key}: ${String(value)}`}</Tag>
          ))}
        </div>
      ) : null}
      {parsed.objects?.length ? (
        <div>
          <Text strong>物品：</Text>
          {parsed.objects.map((item) => (
            <Tag key={item}>{item}</Tag>
          ))}
        </div>
      ) : null}
      {parsed.actions?.length ? (
        <div>
          <Text strong>动作：</Text>
          {parsed.actions.map((item) => (
            <Tag key={item}>{item}</Tag>
          ))}
        </div>
      ) : null}
      {parsed.emotions?.length ? (
        <div>
          <Text strong>情绪：</Text>
          {parsed.emotions.map((item) => (
            <Tag key={item}>{item}</Tag>
          ))}
        </div>
      ) : null}
    </Space>
  )
}

/** 生成结果预览 + 操作（下载 / 重新生成 / 保存到绘本）。 */
function ResultPreview({ task, onRegenerate, onSaveToBook }: ResultPreviewProps) {
  const [downloading, setDownloading] = useState(false)
  const imageId = task.result?.image_id ?? null
  const imageUrl = task.result?.image_url ?? null

  const handleDownload = async () => {
    if (!imageId) {
      return
    }
    setDownloading(true)
    try {
      await imagesApi.download(imageId)
    } catch (error) {
      console.error('下载图片失败', error)
      message.error('下载图片失败')
    } finally {
      setDownloading(false)
    }
  }

  return (
    <Card
      title="生成结果"
      className="!rounded-2xl"
      extra={
        <Space>
          <Button
            size="small"
            icon={<DownloadOutlined />}
            disabled={!imageId}
            loading={downloading}
            onClick={handleDownload}
          >
            下载
          </Button>
          <Button size="small" icon={<ReloadOutlined />} onClick={onRegenerate}>
            重新生成
          </Button>
          <Button
            size="small"
            type="primary"
            icon={<SaveOutlined />}
            disabled={!imageId}
            onClick={() => imageId && onSaveToBook(imageId)}
          >
            保存到绘本
          </Button>
        </Space>
      }
    >
      {imageUrl ? (
        <div className="rounded-xl overflow-hidden bg-slate-50 flex items-center justify-center">
          <img src={imageUrl} alt={task.original_text ?? '生成结果'} className="max-h-[420px] w-auto" />
        </div>
      ) : (
        <Empty description="暂无生成图" />
      )}

      {task.original_text ? (
        <Paragraph type="secondary" className="!mt-4 !mb-2 text-sm">
          原文：{task.original_text}
        </Paragraph>
      ) : null}

      <Collapse
        ghost
        items={[
          {
            key: 'detail',
            label: '查看提示词与解析结果',
            children: (
              <Space direction="vertical" className="w-full">
                <Text type="secondary" className="text-xs break-all">
                  提示词：{task.prompt || '—'}
                </Text>
                {renderParsed(task)}
              </Space>
            ),
          },
        ]}
      />
    </Card>
  )
}

export default ResultPreview
