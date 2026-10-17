import { useCallback, useEffect, useState } from 'react'
import { Link, useNavigate, useSearchParams } from 'react-router-dom'
import { Alert, App, Button, Card, Col, Row, Typography } from 'antd'

import GenerationForm from '../../components/ai/GenerationForm'
import GenerationHistory from '../../components/ai/GenerationHistory'
import GenerationProgress from '../../components/ai/GenerationProgress'
import ResultPreview from '../../components/ai/ResultPreview'
import SaveToBookModal from '../../components/ai/SaveToBookModal'
import AppLayout from '../../components/layout/AppLayout'
import PageHeader from '../../components/layout/PageHeader'
import { aiCredentialsApi } from '../../api/aiCredentials'
import { useGeneration } from '../../hooks/useGeneration'
import { useAuthStore } from '../../stores/authStore'
import { useGenerationStore } from '../../stores/generationStore'
import type { AiCredentialOverview } from '../../types/aiCredentials'
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
  const { message } = App.useApp()
  const navigate = useNavigate()
  const [searchParams] = useSearchParams()
  const bookId = searchParams.get('bookId') ?? undefined
  const isAuthenticated = useAuthStore((state) => state.isAuthenticated)

  const { currentTask, polling, createTask, regenerate, cancel, reset } = useGeneration()
  const fetchHistory = useGenerationStore((state) => state.fetchHistory)

  const [submitting, setSubmitting] = useState(false)
  const [reuse, setReuse] = useState<ReuseState>({ key: 0 })
  const [saveOpen, setSaveOpen] = useState(false)
  const [savingImageId, setSavingImageId] = useState<string | null>(null)
  const [byok, setByok] = useState<AiCredentialOverview | null>(null)

  // 低干扰提示条：BYOK 状态只用于引导，失败时不影响生成流程
  useEffect(() => {
    let cancelled = false
    if (!isAuthenticated) {
      setByok(null)
      return () => {
        cancelled = true
      }
    }
    aiCredentialsApi
      .getOverview()
      .then(({ data }) => {
        if (!cancelled) setByok(data.data)
      })
      .catch((error) => {
        console.error('读取 BYOK 状态失败', error)
      })
    return () => {
      cancelled = true
    }
  }, [isAuthenticated])

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

      <div className="mb-5">
        {!isAuthenticated ? (
          <Alert
            type="info"
            showIcon
            message={
              <span>
                登录后可配置自己的 API Key，生成不占全站额度。
                <Link to="/login">去登录</Link>
              </span>
            }
          />
        ) : byok?.byok_enabled ? (
          byok.quota_exempt ? (
            <Alert
              type="success"
              showIcon
              message={
                <span>
                  本次将使用你的 API Key（文本理解 + 图像生成），不占全站每日额度。
                  <Link to="/profile#ai-credentials">管理 Key</Link>
                </span>
              }
            />
          ) : byok.credentials.llm || byok.credentials.image ? (
            <Alert
              type="info"
              showIcon
              message={
                <span>
                  已启用部分 BYOK：
                  {byok.credentials.llm ? '文本理解' : ''}
                  {byok.credentials.llm && byok.credentials.image ? '、' : ''}
                  {byok.credentials.image ? '图像生成' : ''}
                  使用你的 Key，其余仍用系统 Key；
                  {byok.quota_exempt ? '不占全站额度。' : '仍会计入每日限额。'}
                  <Link to="/profile#ai-credentials">继续配置</Link>
                </span>
              }
            />
          ) : (
            <Alert
              type="info"
              showIcon
              message={
                <span>
                  用自己的 API Key 生成，可免每日限额。
                  <Link to="/profile#ai-credentials">去配置</Link>
                </span>
              }
            />
          )
        ) : null}
      </div>

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
            {currentTask?.status === TaskStatus.FAILED &&
            (currentTask.byok_llm || currentTask.byok_image) ? (
              <Alert
                className="mt-3"
                type="info"
                showIcon
                message={
                  <span>
                    本次使用了你配置的 API Key。若提示 Key 无效或额度不足，请到
                    <Link to="/profile#ai-credentials">个人中心</Link>
                    更新后重试。
                  </span>
                }
              />
            ) : null}
          </div>

          {completed && currentTask ? (
            <div className="mt-6">
              <ResultPreview
                task={currentTask}
                onRegenerate={handleRegenerate}
                onSaveToBook={(imageId) => {
                  // Q3②：游客不能写绘本（后端 MemberFilter 也会 403）；
                  // 直接引导登录，避免进入「你还没有绘本」的死路弹窗
                  if (!isAuthenticated) {
                    message.info('登录后即可保存到绘本')
                    navigate('/login')
                    return
                  }
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
