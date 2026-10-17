import { Button, Input, InputNumber, Popover, Select, Space, Typography } from 'antd'
import { HistoryOutlined, PlusOutlined, SendOutlined, SettingOutlined } from '@ant-design/icons'

import type { ArtStyle } from '../../types/generation'
import { ART_STYLE_OPTIONS, IMAGE_SIZE_OPTIONS, STORY_TEXT_MAX_LENGTH } from '../../utils/constants'

const { Text } = Typography
const { TextArea } = Input

export interface ComposerProps {
  value: string
  onValueChange: (value: string) => void
  style: ArtStyle
  onStyleChange: (value: ArtStyle) => void
  size: string
  onSizeChange: (value: string) => void
  count: number
  onCountChange: (value: number) => void
  seed: number | null
  onSeedChange: (value: number | null) => void
  isGuest: boolean
  sending: boolean
  onSend: () => void
  onHistory: () => void
  onNew: () => void
}

/** 底部创作工具条：关键参数常驻可见，低频参数收进「更多」。 */
function Composer(props: ComposerProps) {
  const { isGuest } = props

  const countOptions = (isGuest ? [1] : [1, 2, 4, 6]).map((value) => ({
    value,
    label: value === 1 ? '1 张' : `${value} 张`,
  }))

  const moreContent = (
    <div className="w-56">
      <label className="mb-1 block text-xs text-ink2" htmlFor="chat-seed">
        随机种子（可选）
      </label>
      <InputNumber
        id="chat-seed"
        min={0}
        max={2147483647}
        className="w-full"
        value={props.seed ?? undefined}
        placeholder="留空自动，同批次共用"
        onChange={(value) => props.onSeedChange(typeof value === 'number' ? value : null)}
      />
      <Text type="secondary" className="mt-2 block text-xs">
        固定种子有助于保持同批次画风一致；不支持种子的厂商会自动忽略。
      </Text>
    </div>
  )

  return (
    <div className="rounded-card border border-hairline bg-paper p-3 shadow-card">
      <div className="mb-2 flex flex-wrap items-center gap-2">
        <Select
          aria-label="艺术风格"
          size="small"
          value={props.style}
          onChange={props.onStyleChange}
          options={ART_STYLE_OPTIONS.map((item) => ({ value: item.value, label: item.label }))}
          className="min-w-[118px]"
        />
        <Select
          aria-label="图片尺寸"
          size="small"
          value={props.size}
          onChange={props.onSizeChange}
          options={IMAGE_SIZE_OPTIONS.map((item) => ({ value: item.value, label: item.label }))}
          className="min-w-[150px]"
        />
        <Select
          aria-label="生成数量"
          size="small"
          value={props.count}
          onChange={props.onCountChange}
          options={countOptions}
          disabled={isGuest}
          className="min-w-[82px]"
        />
        {isGuest ? (
          <Text type="secondary" className="text-xs">
            登录后可一次生成最多 6 张
          </Text>
        ) : null}
        <Popover content={moreContent} trigger="click" placement="topRight">
          <Button size="small" icon={<SettingOutlined />}>
            更多
          </Button>
        </Popover>
        <div className="ml-auto flex items-center gap-1">
          <Button size="small" icon={<HistoryOutlined />} onClick={props.onHistory}>
            历史
          </Button>
          <Button size="small" icon={<PlusOutlined />} onClick={props.onNew}>
            新对话
          </Button>
        </div>
      </div>

      <div className="flex items-end gap-2">
        <TextArea
          value={props.value}
          onChange={(event) => props.onValueChange(event.target.value)}
          placeholder="讲讲你的故事，例如：小女孩在雨后的森林里发现一只受伤的小鸟……"
          autoSize={{ minRows: 1, maxRows: 5 }}
          maxLength={STORY_TEXT_MAX_LENGTH}
          onPressEnter={(event) => {
            if (!event.shiftKey) {
              event.preventDefault()
              props.onSend()
            }
          }}
          className="!resize-none"
        />
        <Button
          type="primary"
          shape="circle"
          size="large"
          icon={<SendOutlined />}
          loading={props.sending}
          disabled={!props.value.trim()}
          onClick={props.onSend}
          aria-label="发送"
        />
      </div>

      <Space size={12} className="mt-2 text-xs text-ink3">
        <span>Enter 发送 · Shift+Enter 换行</span>
        <span>{props.value.length}/{STORY_TEXT_MAX_LENGTH}</span>
      </Space>
    </div>
  )
}

export default Composer
