import { useCallback, useEffect, useState } from 'react'
import { useNavigate, useParams } from 'react-router-dom'
import {
  Button,
  Card,
  Col,
  Descriptions,
  Empty,
  Image,
  Popconfirm,
  Result,
  Row,
  Spin,
  Tag,
  Typography,
  message,
} from 'antd'
import {
  ArrowLeftOutlined,
  DeleteOutlined,
  EditOutlined,
  ReloadOutlined,
  RocketOutlined,
} from '@ant-design/icons'

import ExportPdfButton from '../../components/book/ExportPdfButton'
import BookFormModal, { type BookFormValues } from '../../components/book/BookFormModal'
import { pagesApi } from '../../api/pages'
import { useBookStore } from '../../stores/bookStore'
import type { BookPage } from '../../types/page'
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

  const [pages, setPages] = useState<BookPage[]>([])
  const [pagesLoading, setPagesLoading] = useState(false)

  const loadPages = useCallback((bookId: string) => {
    setPagesLoading(true)
    pagesApi
      .list(bookId)
      .then(({ data }) => setPages(data.data ?? []))
      .catch((error) => console.error('加载绘本页失败', error))
      .finally(() => setPagesLoading(false))
  }, [])

  useEffect(() => {
    if (id) {
      fetchBook(id)
      loadPages(id)
    }
  }, [id, fetchBook, loadPages])

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
            <Descriptions.Item label="已保存页数">{pages.length}</Descriptions.Item>
            <Descriptions.Item label="更新时间">
              {formatDateTime(current.updated_at, 'YYYY-MM-DD HH:mm:ss')}
            </Descriptions.Item>
          </Descriptions>
        </Card>

        <Card
          className="!rounded-2xl !mt-6"
          title="绘本页面"
          extra={
            <div className="flex items-center gap-2">
              <Button
                size="small"
                icon={<ReloadOutlined />}
                onClick={() => id && loadPages(id)}
              >
                刷新
              </Button>
              <ExportPdfButton bookTitle={current.title} pages={pages} />
            </div>
          }
        >
          <div className="flex flex-wrap items-center gap-3 mb-4">
            <Button
              type="primary"
              icon={<RocketOutlined />}
              onClick={() => navigate(`/generate?bookId=${current.id}`)}
            >
              为该绘本生成插画
            </Button>
            <Paragraph type="secondary" className="!mb-0 text-sm">
              生成结果在「生成页」点击「保存到绘本」即可追加到本绘本，随后可导出多页 PDF。
            </Paragraph>
          </div>

          <Spin spinning={pagesLoading}>
            {pages.length === 0 ? (
              <Empty description="还没有保存任何页面，先为绘本生成并保存插画吧" />
            ) : (
              <Row gutter={[16, 16]}>
                {pages.map((page) => (
                  <Col key={page.id} xs={12} sm={8} md={6}>
                    <div className="rounded-xl overflow-hidden border border-slate-100 bg-slate-50">
                      {page.image_url ? (
                        <Image
                          src={page.image_url}
                          alt={page.title ?? `第 ${page.page_number} 页`}
                          className="object-cover"
                          width="100%"
                          height={140}
                        />
                      ) : (
                        <div className="h-[140px] flex items-center justify-center text-slate-400 text-xs">
                          无图
                        </div>
                      )}
                      <div className="px-2 py-2">
                        <Text className="text-xs block truncate" strong>
                          {page.page_number}. {page.title || '未命名'}
                        </Text>
                      </div>
                    </div>
                  </Col>
                ))}
              </Row>
            )}
          </Spin>
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
