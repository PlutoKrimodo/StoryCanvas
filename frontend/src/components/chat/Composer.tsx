import { useState } from 'react'
import { Button, Input, InputNumber, Popover, Select, Space, Typography } from 'antd'
import {
  BookOutlined,
  HistoryOutlined,
  PlusOutlined,
  RobotOutlined,
  SendOutlined,
  SettingOutlined,
} from '@ant-design/icons'

import type { ArtStyle } from '../../types/generation'
import { ART_STYLE_OPTIONS, IMAGE_SIZE_OPTIONS, STORY_TEXT_MAX_LENGTH } from '../../utils/constants'

const { Text } = Typography
const { TextArea } = Input

/** 双项自定义模型（BYOK）时的单次生成上限，与后端 12 张保持一致 */
const BYOK_MAX_COUNT = 12
/** 使用系统模型时的单次生成上限 */
const SYSTEM_MAX_COUNT = 6

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
  /** 打开「绘本」列表（仅登录用户） */
  onBooks?: () => void
  /** 打开「模型管理」弹窗（仅登录用户） */
  onModelManager?: () => void
  /** 文本与图像两项都使用自定义模型：允许自定义张数 */
  byokDual?: boolean
  /** 极简态：隐藏「历史/新对话」，仅文本框 + 发送钮（首页态使用） */
  minimal?: boolean
  /** 受控参数可见性；不传则由聚焦/输入自动唤醒（工作室态传 true 保持常驻） */
  paramsVisible?: boolean
}

/**
 * 底部创作工具条。
 *
 * 参数行默认隐藏，用户聚焦输入框或开始输入文本时以高度 + 透明度过渡「唤醒」，
 * 营造搜索式微交互；极简态（首页）下不再展示「历史/新对话」。
 */
function Composer(props: ComposerProps) {
  const { isGuest, minimal, byokDual } = props
  const [awake, setAwake] = useState(false)
  // 受控时以父级为准（工作室态常驻），否则由聚焦/输入自动唤醒
  const paramsVisible = props.paramsVisible ?? awake

  const wake = () => setAwake(true)

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
      {/* 参数行：聚焦或输入后唤醒 */}
      <div
        className={`grid transition-all duration-300 ease-out motion-reduce:transition-none ${
          paramsVisible ? 'grid-rows-[1fr] opacity-100' : 'grid-rows-[0fr] opacity-0'
        }`}
      >
        <div className="overflow-hidden">
          <div className="flex flex-wrap items-center gap-2 pb-2 pt-1">
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

            {byokDual ? (
              <Space size={4}>
                <InputNumber
                  aria-label="生成数量"
                  size="small"
                  min={1}
                  max={BYOK_MAX_COUNT}
                  value={props.count}
                  onChange={(value) => props.onCountChange(typeof value === 'number' ? value : 1)}
                  className="!w-[72px]"
                />
                <Text type="secondary" className="text-xs">
                  张 · 自定义模型
                </Text>
              </Space>
            ) : (
              <>
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
                    游客 1 张 · 登录后最多 {SYSTEM_MAX_COUNT} 张
                  </Text>
                ) : (
                  <Text type="secondary" className="text-xs">
                    系统模型最多 {SYSTEM_MAX_COUNT} 张 · 配置自定义模型后可自定义
                  </Text>
                )}
              </>
            )}

            {!isGuest && props.onBooks ? (
              <Button
                size="small"
                icon={<BookOutlined />}
                onClick={props.onBooks}
                aria-label="绘本"
              >
                绘本
              </Button>
            ) : null}
            {!isGuest && props.onModelManager ? (
              <Button
                size="small"
                icon={<RobotOutlined />}
                onClick={props.onModelManager}
                aria-label="模型管理"
              >
                模型管理
              </Button>
            ) : null}

            <Popover content={moreContent} trigger="click" placement="topRight">
              <Button size="small" icon={<SettingOutlined />}>
                更多
              </Button>
            </Popover>

            {!minimal ? (
              <div className="ml-auto flex items-center gap-1">
                <Button size="small" icon={<HistoryOutlined />} onClick={props.onHistory}>
                  历史
                </Button>
                <Button size="small" icon={<PlusOutlined />} onClick={props.onNew}>
                  新对话
                </Button>
              </div>
            ) : null}
          </div>
        </div>
      </div>

      <div className="flex items-end gap-2">
        <TextArea
          value={props.value}
          onFocus={wake}
          onChange={(event) => {
            if (event.target.value) wake()
            props.onValueChange(event.target.value)
          }}
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
        <span>
          {props.value.length}/{STORY_TEXT_MAX_LENGTH}
        </span>
      </Space>
    </div>
  )
}

export default Composer
