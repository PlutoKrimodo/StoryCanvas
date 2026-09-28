import { useEffect, useState } from 'react'
import { useNavigate, useParams } from 'react-router-dom'
import { Button, Card, Descriptions, Popconfirm, Result, Spin, Tag, Typography, message } from 'antd'
import { ArrowLeftOutlined, DeleteOutlined, EditOutlined, RocketOutlined } from '@ant-design/icons'

import BookFormModal, { type BookFormValues } from '../../components/book/BookFormModal'
import { useBookStore } from '../../stores/bookStore'
import { BOOK_STATUS_COLOR, BOOK_STATUS_TEXT } from '../../utils/constants'
import { formatDateTime } from '../../utils/datetime'

const { Title, Paragraph, Text } = Typography

function BookDetail() {
  const { id } = useParams<{ id: string }>()
  const navigate = useNavigate()
  const { current, detailLoading, fetchBook, updateBook, removeBook } = useBookStore()

  const [editOpen, setEditOpen] = useState(false)
  const [saving, setSaving] = useState(false)
  const [deleting, setDeleting] = useState(false)

  useEffect(() => {
    if (id) {
      fetchBook(id)
    }
  }, [id, fetchBook])

  const handleUpdate = (values: BookFormValues) => {
    if (!id) {
      return
    }
    setSaving(true)
    updateBook(id, {
      title: values.title.trim(),
      description: values.description?.trim() ?? '',
      status: values.status,
    })
      .then((book) => {
        if (book) {
          message.success('绘本已更新')
          setEditOpen(false)
        }
      })
      .finally(() => setSaving(false))
  }

  const handleDelete = () => {
    if (!id) {
      return
    }
    setDeleting(true)
    removeBook(id)
      .then((ok) => {
        if (ok) {
          message.success('绘本已删除')
          navigate('/books', { replace: true })
        }
      })
      .finally(() => setDeleting(false))
  }

  if (detailLoading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-[#F5F3FF] via-white to-[#EEF2FF]">
        <Spin size="large" />
      </div>
    )
  }

  if (!current) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-[#F5F3FF] via-white to-[#EEF2FF] px-4">
        <Result
          status="404"
          title="绘本不存在"
          subTitle="该绘本不存在，或你没有访问权限"
          extra={
            <Button type="primary" onClick={() => navigate('/books')}>
              返回绘本列表
            </Button>
          }
        />
      </div>
    )
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-[#F5F3FF] via-white to-[#EEF2FF] px-4 py-8">
      <div className="max-w-4xl mx-auto">
        <Button
          type="text"
          icon={<ArrowLeftOutlined />}
          className="!px-0 mb-4"
          onClick={() => navigate('/books')}
        >
          返回绘本列表
        </Button>

        <Card className="!rounded-2xl shadow-sm">
          <div className="flex flex-wrap items-start justify-between gap-4">
            <div>
              <div className="flex items-center gap-3">
                <Title level={3} className="!mb-0">
                  {current.title}
                </Title>
                <Tag color={BOOK_STATUS_COLOR[current.status]}>
                  {BOOK_STATUS_TEXT[current.status]}
                </Tag>
              </div>
              <Text type="secondary" className="text-sm">
                创建于 {formatDateTime(current.created_at, 'YYYY-MM-DD HH:mm')}
              </Text>
            </div>
            <div className="flex items-center gap-2">
              <Button icon={<EditOutlined />} onClick={() => setEditOpen(true)}>
                编辑
              </Button>
              <Popconfirm
                title="删除这本绘本？"
                description="删除后不可恢复，其下内容将一并移除。"
                okText="删除"
                cancelText="取消"
                okButtonProps={{ danger: true, loading: deleting }}
                onConfirm={handleDelete}
              >
                <Button danger icon={<DeleteOutlined />}>
                  删除
                </Button>
              </Popconfirm>
            </div>
          </div>

          <Descriptions column={1} className="mt-6" bordered size="middle">
            <Descriptions.Item label="描述">{current.description || '暂无描述'}</Descriptions.Item>
            <Descriptions.Item label="状态">{BOOK_STATUS_TEXT[current.status]}</Descriptions.Item>
            <Descriptions.Item label="更新时间">
              {formatDateTime(current.updated_at, 'YYYY-MM-DD HH:mm:ss')}
            </Descriptions.Item>
          </Descriptions>
        </Card>

        <Card className="!rounded-2xl !mt-6 border-dashed">
          <div className="flex items-center gap-4">
            <div className="h-14 w-14 rounded-xl bg-gradient-to-br from-violet-400 to-indigo-500 flex items-center justify-center shrink-0">
              <RocketOutlined className="text-2xl text-white" />
            </div>
            <div>
              <Text strong className="text-base">
                AI 绘本插画
              </Text>
              <Paragraph type="secondary" className="!mb-0">
                智能图像生成能力将在后续里程碑接入，届时可在此为绘本生成专属插画。
              </Paragraph>
            </div>
          </div>
        </Card>
      </div>

      <BookFormModal
        open={editOpen}
        book={current}
        confirmLoading={saving}
        onCancel={() => setEditOpen(false)}
        onSubmit={handleUpdate}
      />
    </div>
  )
}

export default BookDetail
