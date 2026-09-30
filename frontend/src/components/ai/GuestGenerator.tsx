import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Alert, Button, Card, Typography } from 'antd'
import { DownloadOutlined, LoginOutlined } from '@ant-design/icons'

import GenerationForm from './GenerationForm'
import GenerationProgress from './GenerationProgress'
import { imagesApi } from '../../api/images'
import { useGeneration } from '../../hooks/useGeneration'
import { useAuthStore } from '../../stores/authStore'
import { TaskStatus, type GenerationRequest } from '../../types/generation'
import { GUEST_GENERATION_LIMIT } from '../../utils/constants'

const { Text, Paragraph } = Typography

/**
 * 游客生成器（Guest-first 能力层）：
 * 未登录用户也能直接体验生成；额度用尽后引导登录（登录即「认领」其图片）。
 */
function GuestGenerator() {
  const navigate = useNavigate()
  const { currentTask, polling, createTask, cancel } = useGeneration()
  const guestQuota = useAuthStore((state) => state.guestQuota)

  const [submitting, setSubmitting] = useState(false)
  const [downloading, setDownloading] = useState(false)

  const quota = guestQuota > 0 ? guestQuota : GUEST_GENERATION_LIMIT
  const completed = currentTask?.status === TaskStatus.COMPLETED && !!currentTask.result?.image_url
  const failed = currentTask?.status === TaskStatus.FAILED

  const handleSubmit = async (payload: GenerationRequest) => {
    setSubmitting(true)
    try {
      await createTask(payload)
    } catch (error) {
      // 429（额度用尽）与其它错误已由 axios 拦截器统一提示
      console.error('游客生成失败', error)
    } finally {
      setSubmitting(false)
    }
  }

  const handleDownload = async () => {
    const imageId = currentTask?.result?.image_id
    if (!imageId) {
      return
    }
    setDownloading(true)
    try {
      await imagesApi.download(imageId)
    } catch (error) {
      console.error('下载图片失败', error)
    } finally {
      setDownloading(false)
    }
  }

  return (
    <Card className="!rounded-2xl shadow-sm">
      <div className="mb-4">
        <Text strong>游客体验</Text>
        <Paragraph type="secondary" className="!mb-0 text-sm">
          无需注册，即可免费生成 {quota} 张插画；登录后可保存到绘本并继续生成。
        </Paragraph>
      </div>

      <GenerationForm
        loading={submitting}
        onSubmit={handleSubmit}
        submitText="立即生成（免费）"
      />

      {currentTask ? (
        <div className="mt-5">
          <GenerationProgress
            task={currentTask}
            polling={polling}
            onCancel={() => void cancel(currentTask.task_id)}
          />
        </div>
      ) : null}

      {completed && currentTask ? (
        <div className="mt-4">
          <div className="rounded-xl overflow-hidden bg-slate-50 flex items-center justify-center">
            <img
              src={currentTask.result!.image_url!}
              alt={currentTask.original_text ?? '生成结果'}
              className="max-h-[360px] w-auto"
            />
          </div>
          <div className="flex flex-wrap items-center justify-between gap-3 mt-3">
            <Button icon={<DownloadOutlined />} loading={downloading} onClick={handleDownload}>
              下载图片
            </Button>
            <Button type="primary" icon={<LoginOutlined />} onClick={() => navigate('/login')}>
              登录后保存到绘本
            </Button>
          </div>
        </div>
      ) : null}

      {failed ? (
        <Alert
          className="mt-4"
          type="warning"
          showIcon
          message="生成未成功"
          description="游客额度可能已用完；登录后可保存图片到绘本，并解锁更多生成次数。"
          action={
            <Button size="small" type="primary" onClick={() => navigate('/login')}>
              去登录
            </Button>
          }
        />
      ) : null}
    </Card>
  )
}

export default GuestGenerator
