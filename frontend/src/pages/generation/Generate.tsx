import { useCallback, useState } from 'react'
import { useSearchParams } from 'react-router-dom'
import { Button, Card, Col, Row, Typography, message } from 'antd'

import GenerationForm from '../../components/ai/GenerationForm'
import GenerationHistory from '../../components/ai/GenerationHistory'
import GenerationProgress from '../../components/ai/GenerationProgress'
import ResultPreview from '../../components/ai/ResultPreview'
import SaveToBookModal from '../../components/ai/SaveToBookModal'
import AppLayout from '../../components/layout/AppLayout'
import PageHeader from '../../components/layout/PageHeader'
import { useGeneration } from '../../hooks/useGeneration'
import { useGenerationStore } from '../../stores/generationStore'
import { ArtStyle, TaskStatus, type GenerationRequest, type TaskResult } from '../../types/generation'

const { Paragraph, Text } = Typography

interface ReuseState {
  key: number
  text?: string
  style?: ArtStyle
}

/**
 * AI 生成页（核心页面）。
 *
 * 支持 `?bookId=xxx` 直接为某本绘本生成；不带参数时为通用生成，
 * 结果可事后通过「保存到绘本」落到任意绘本（T-1 解耦设计）。
 */
function Generate() {
  const [searchParams] = useSearchParams()
  const bookId = searchParams.get('bookId') ?? undefined

  const { currentTask, polling, createTask, regenerate, cancel, reset } = useGeneration()
  const fetchHistory = useGenerationStore((state) => state.fetchHistory)

  const [submitting, setSubmitting] = useState(false)
  const [reuse, setReuse] = useState<ReuseState>({ key: 0 })
  const [saveOpen, setSaveOpen] = useState(false)
  const [savingImageId, setSavingImageId] = useState<string | null>(null)

  const handleSubmit = async (payload: GenerationRequest) => {
    setSubmitting(true)
    try {
      await createTask(payload)
      message.success('生成任务已创建，正在为你绘制插画…')
    } catch (error) {
      console.error('创建生成任务失败', error)
      message.error(error instanceof Error ? error.message : '创建生成任务失败')
    } finally {
      setSubmitting(false)
    }
  }

  const handleRegenerate = useCallback(() => {
    if (!currentTask) {
      return
    }
    regenerate(currentTask.task_id)
      .then(() => message.success('已提交重新生成'))
      .catch((error) => {
        console.error('重新生成失败', error)
        message.error(error instanceof Error ? error.message : '重新生成失败')
      })
  }, [currentTask, regenerate])

  const handleCancel = useCallback(() => {
    if (!currentTask) {
      return
    }
    cancel(currentTask.task_id).then((ok) => {
      if (ok) {
        message.info('已请求取消任务')
      }
    })
  }, [currentTask, cancel])

  const handleReuse = useCallback((task: TaskResult) => {
    const style = task.parameters?.style
    setReuse((prev) => ({
      key: prev.key + 1,
      text: task.original_text ?? undefined,
      style: typeof style === 'string' ? (style as ArtStyle) : undefined,
    }))
    message.info('已回填历史参数，可修改后重新生成')
  }, [])

  const completed = currentTask?.status === TaskStatus.COMPLETED && !!currentTask.result?.image_url

  return (
    <AppLayout variant="app">
      <PageHeader
        title="AI 绘本插画生成"
        description={
          <>
            输入一段故事文字，选择艺术风格，系统将解析文本、优化提示词并生成插画。
            {bookId ? <Text strong> 本次结果将绑定到当前绘本。</Text> : null}
          </>
        }
        extra={currentTask ? <Button onClick={reset}>重新开始</Button> : undefined}
      />

      <Row gutter={[24, 24]}>
        <Col xs={24} lg={14}>
          {/* GenerationForm 不再自带 Card，外壳由此处提供（docs/17 §4.1 容器嵌套修正） */}
          <Card className="shadow-card">
            <GenerationForm
              key={reuse.key}
              bookId={bookId}
              initialText={reuse.text}
              initialStyle={reuse.style}
              loading={submitting}
              onSubmit={handleSubmit}
            />
          </Card>

          <div className="mt-6">
            <GenerationProgress
              task={currentTask}
              polling={polling}
              onCancel={handleCancel}
              onRegenerate={completed || currentTask?.status === TaskStatus.FAILED ? handleRegenerate : undefined}
            />
          </div>

          {completed && currentTask ? (
            <div className="mt-6">
              <ResultPreview
                task={currentTask}
                onRegenerate={handleRegenerate}
                onSaveToBook={(imageId) => {
                  setSavingImageId(imageId)
                  setSaveOpen(true)
                }}
              />
            </div>
          ) : null}

          {!currentTask ? (
            <Card className="!mt-6 border-dashed">
              <Paragraph type="secondary" className="!mb-0">
                生成结果会显示在这里。你可以先自由生成，再决定保存到哪本绘本。
              </Paragraph>
            </Card>
          ) : null}
        </Col>

        <Col xs={24} lg={10}>
          <GenerationHistory bookId={bookId} onReuse={handleReuse} />
        </Col>
      </Row>

      <SaveToBookModal
        open={saveOpen}
        imageId={savingImageId}
        defaultBookId={bookId}
        onCancel={() => setSaveOpen(false)}
        onSaved={() => {
          // 保存成功后刷新历史，便于确认本次结果已入库
          void fetchHistory({ book_id: bookId })
        }}
      />
    </AppLayout>
  )
}

export default Generate
