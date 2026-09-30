import { useEffect, useState } from 'react'
import { Button, Card, Collapse, Form, Input, InputNumber, Select, Space } from 'antd'
import { BulbOutlined, ThunderboltOutlined } from '@ant-design/icons'

import StyleSelector from './StyleSelector'
import type { GenerationRequest } from '../../types/generation'
import { ArtStyle } from '../../types/generation'
import { IMAGE_SIZE_OPTIONS, STORY_TEXT_MAX_LENGTH } from '../../utils/constants'

const { TextArea } = Input

/** 表单内部字段（parameters 在提交时组装） */
interface GenerationFormValues {
  text: string
  style: ArtStyle
  size?: string
  seed?: number
}

interface GenerationFormProps {
  /** 可选：绑定到某个绘本；不传即为「先生成、后保存」的解耦流程 */
  bookId?: string
  initialText?: string
  initialStyle?: ArtStyle
  submitText?: string
  loading?: boolean
  onSubmit: (payload: GenerationRequest) => void | Promise<void>
}

const SAMPLE_TEXT = '小女孩在雨后的森林里发现了一只受伤的小鸟，她轻轻把它捧在手心，带它回家疗伤。'

/**
 * AI 生成表单（**可复用组件**）。
 *
 * 按 TODO T-1 的决策，生成动作与「保存到绘本」解耦：
 * 组件不强制依赖 bookId，便于后续搬到首页做「游客优先」体验。
 */
function GenerationForm({
  bookId,
  initialText,
  initialStyle,
  submitText = '生成插画',
  loading = false,
  onSubmit,
}: GenerationFormProps) {
  const [form] = Form.useForm<GenerationFormValues>()
  const [advancedOpen, setAdvancedOpen] = useState(false)

  // 外部回填（例如从历史记录「复用参数」）
  useEffect(() => {
    if (initialText !== undefined) {
      form.setFieldValue('text', initialText)
    }
  }, [initialText, form])

  useEffect(() => {
    if (initialStyle !== undefined) {
      form.setFieldValue('style', initialStyle)
    }
  }, [initialStyle, form])

  const handleFinish = (values: GenerationFormValues) => {
    const payload: GenerationRequest = {
      text: values.text.trim(),
      style: values.style,
    }
    if (bookId) {
      payload.book_id = bookId
    }
    const parameters: GenerationRequest['parameters'] = {}
    if (values.size) {
      parameters.size = values.size
    }
    if (typeof values.seed === 'number') {
      parameters.seed = values.seed
    }
    if (Object.keys(parameters).length > 0) {
      payload.parameters = parameters
    }
    void onSubmit(payload)
  }

  const fillSample = () => {
    form.setFieldsValue({ text: SAMPLE_TEXT })
  }

  return (
    <Card
      title="故事描述"
      className="!rounded-2xl"
      extra={
        <Button type="link" size="small" icon={<BulbOutlined />} onClick={fillSample}>
          填入示例
        </Button>
      }
    >
      <Form
        form={form}
        layout="vertical"
        requiredMark={false}
        initialValues={{ style: initialStyle ?? ArtStyle.CARTOON, size: '1024x1024' }}
        onFinish={handleFinish}
      >
        <Form.Item
          name="text"
          rules={[
            { required: true, message: '请输入故事描述' },
            { max: STORY_TEXT_MAX_LENGTH, message: `不能超过 ${STORY_TEXT_MAX_LENGTH} 个字符` },
          ]}
        >
          <TextArea
            rows={5}
            maxLength={STORY_TEXT_MAX_LENGTH}
            showCount
            placeholder="例如：小女孩在雨后的森林里发现了一只受伤的小鸟。"
          />
        </Form.Item>

        <Form.Item
          name="style"
          label="艺术风格"
          rules={[{ required: true, message: '请选择艺术风格' }]}
        >
          <StyleSelector />
        </Form.Item>

        <Collapse
          ghost
          activeKey={advancedOpen ? ['advanced'] : []}
          onChange={(keys) => setAdvancedOpen(keys.length > 0)}
          items={[
            {
              key: 'advanced',
              label: '高级参数（尺寸 / 随机种子）',
              children: (
                <Space align="start" wrap size="large">
                  <Form.Item name="size" label="图片尺寸" className="!mb-0">
                    <SizeField />
                  </Form.Item>
                  <Form.Item name="seed" label="随机种子" className="!mb-0">
                    <InputNumber
                      min={0}
                      max={2147483647}
                      placeholder="留空则随机"
                      style={{ width: 180 }}
                    />
                  </Form.Item>
                </Space>
              ),
            },
          ]}
        />

        <Form.Item className="!mt-6 !mb-0">
          <Button
            type="primary"
            htmlType="submit"
            loading={loading}
            icon={<ThunderboltOutlined />}
            block
            size="large"
          >
            {submitText}
          </Button>
        </Form.Item>
      </Form>
    </Card>
  )
}

/** 尺寸下拉（独立小组件，保持主表单可读性） */
function SizeField() {
  return (
    <Select
      options={IMAGE_SIZE_OPTIONS.map((option) => ({
        value: option.value,
        label: option.label,
      }))}
      style={{ width: 240 }}
    />
  )
}

export default GenerationForm
