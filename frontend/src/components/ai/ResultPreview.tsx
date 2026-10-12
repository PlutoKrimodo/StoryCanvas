import { useState } from 'react'
import { App, Button, Card, Collapse, Empty, Typography } from 'antd'
import { DownloadOutlined, ReloadOutlined, SaveOutlined } from '@ant-design/icons'

import ArtImage from '../common/ArtImage'
import { imagesApi } from '../../api/images'
import type { TaskResult } from '../../types/generation'
import { LOGO_COLORS } from '../../theme/palette'

const { Text } = Typography

interface ResultPreviewProps {
  task: TaskResult
  onRegenerate: () => void
  /** 传入该图 ID 以打开「保存到绘本」 */
  onSaveToBook: (imageId: string) => void
}

interface ArchiveRow {
  label: string
  values: string[]
}

/** parsed_data.scene 的字段名来自提示词模板，展示时中文化以免档案里混排英文键 */
const SCENE_LABELS: Record<string, string> = {
  location: '地点',
  time: '时间',
  weather: '天气',
  mood: '氛围',
}

function buildRows(task: TaskResult): ArchiveRow[] {
  const parsed = task.parsed_data
  if (!parsed) {
    return []
  }

  const sceneEntries = Object.entries(parsed.scene ?? {}).filter(([, value]) => !!value)

  return [
    { label: '角色', values: (parsed.characters ?? []).map((character) => character.name) },
    { label: '场景', values: sceneEntries.map(([key, value]) => `${SCENE_LABELS[key] ?? key}：${String(value)}`) },
    { label: '物品', values: parsed.objects ?? [] },
    { label: '动作', values: parsed.actions ?? [] },
    { label: '情绪', values: parsed.emotions ?? [] },
  ].filter((row) => row.values.length > 0)
}

/**
 * 解析结果档案化：五组 Tag 收成「标签 + 圆点值」五行。
 * 圆点沿用 logo 五色并跨分组连续轮转，避免每行都从同一色开始。
 */
function renderArchive(rows: ArchiveRow[]) {
  if (!rows.length) {
    return <Text type="secondary">暂无结构化解析结果</Text>
  }

  let cursor = 0
  const dotted = rows.map((row) => ({
    ...row,
    dots: row.values.map(() => LOGO_COLORS[cursor++ % LOGO_COLORS.length]),
  }))

  return (
    <div className="flex flex-col gap-y-2">
      {dotted.map((row) => (
        <div key={row.label} className="flex gap-3">
          <span className="w-8 shrink-0 pt-[3px] text-xs text-ink3">{row.label}</span>
          <span className="flex flex-wrap gap-x-4 gap-y-1">
            {row.values.map((value, index) => (
              <span key={value} className="inline-flex items-center gap-1.5 text-sm text-ink2">
                <span
                  aria-hidden
                  className="h-[3px] w-[3px] rounded-full"
                  style={{ backgroundColor: row.dots[index] }}
                />
                {value}
              </span>
            ))}
          </span>
        </div>
      ))}
    </div>
  )
}

/** 生成结果预览 + 操作（保存到绘本优先，下载/重新生成次之）。 */
function ResultPreview({ task, onRegenerate, onSaveToBook }: ResultPreviewProps) {
  const { message } = App.useApp()
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
    <Card title="生成结果">
      {imageUrl ? (
        <ArtImage
          src={imageUrl}
          alt={task.original_text ?? '生成结果'}
          wrapperClassName="rounded-card"
          maxHeight={420}
        />
      ) : (
        <Empty description="暂无生成图" />
      )}

      {/* 操作条紧贴图片：转化动作（保存到绘本）为唯一 primary */}
      <div className="mt-4 flex flex-wrap items-center gap-2">
        <Button
          type="primary"
          icon={<SaveOutlined />}
          disabled={!imageId}
          onClick={() => imageId && onSaveToBook(imageId)}
        >
          保存到绘本
        </Button>
        <Button
          icon={<DownloadOutlined />}
          disabled={!imageId}
          loading={downloading}
          onClick={handleDownload}
        >
          下载
        </Button>
        <Button type="link" icon={<ReloadOutlined />} onClick={onRegenerate}>
          重新生成
        </Button>
      </div>

      {task.original_text ? (
        <div className="mt-4 border-l-2 border-clay/30 pl-3">
          <Text className="text-sm leading-relaxed text-ink2">{task.original_text}</Text>
        </div>
      ) : null}

      <Collapse
        ghost
        className="mt-4"
        items={[
          {
            key: 'detail',
            label: '查看提示词与解析结果',
            children: (
              <div className="flex flex-col gap-y-3">
                <Text type="secondary" className="text-xs leading-relaxed break-all">
                  提示词：{task.prompt || '—'}
                </Text>
                {renderArchive(buildRows(task))}
              </div>
            ),
          },
        ]}
      />
    </Card>
  )
}

export default ResultPreview
