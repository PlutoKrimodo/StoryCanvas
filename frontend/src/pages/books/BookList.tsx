import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Button, Empty, Pagination, Spin, Tabs, Typography, message } from 'antd'
import { HomeOutlined, PlusOutlined } from '@ant-design/icons'

import BookCard from '../../components/book/BookCard'
import BookFormModal, { type BookFormValues } from '../../components/book/BookFormModal'
import { useBookStore } from '../../stores/bookStore'
import type { Book, BookStatus, CreateBookRequest } from '../../types/book'
import { BOOK_STATUS_OPTIONS } from '../../utils/constants'

const { Title, Paragraph } = Typography

type StatusFilter = BookStatus | 'all'

const STATUS_TABS = [
  { key: 'all', label: '全部' },
  ...BOOK_STATUS_OPTIONS.map((option) => ({ key: option.value, label: option.label })),
]

function BookList() {
  const navigate = useNavigate()
  const { books, total, page, limit, loading, fetchBooks, createBook, updateBook } = useBookStore()

  const [statusFilter, setStatusFilter] = useState<StatusFilter>('all')
  const [modalOpen, setModalOpen] = useState(false)
  const [editing, setEditing] = useState<Book | null>(null)
  const [saving, setSaving] = useState(false)

  const queryStatus = statusFilter === 'all' ? undefined : statusFilter

  useEffect(() => {
    fetchBooks({ page: 1, status: queryStatus })
  }, [fetchBooks, queryStatus])

  const openCreate = () => {
    setEditing(null)
    setModalOpen(true)
  }

  const openEdit = (book: Book) => {
    setEditing(book)
    setModalOpen(true)
  }

  const closeModal = () => {
    setModalOpen(false)
    setEditing(null)
  }

  const handleSubmit = (values: BookFormValues) => {
    const payload: CreateBookRequest = {
      title: values.title.trim(),
      description: values.description?.trim() ?? '',
      status: values.status,
    }
    setSaving(true)
    const action = editing ? updateBook(editing.id, payload) : createBook(payload)
    action
      .then((book) => {
        if (!book) {
          return undefined
        }
        message.success(editing ? '绘本已更新' : '绘本创建成功')
        closeModal()
        return fetchBooks({ page: editing ? page : 1, status: queryStatus })
      })
      .finally(() => setSaving(false))
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-[#F5F3FF] via-white to-[#EEF2FF] px-4 py-8">
      <div className="max-w-6xl mx-auto">
        <div className="flex flex-wrap items-center justify-between gap-4">
          <div>
            <Title level={2} className="!mb-1">
              我的绘本
            </Title>
            <Paragraph type="secondary" className="!mb-0">
              用一段故事文字，生成属于你的儿童绘本插画
            </Paragraph>
          </div>
          <div className="flex items-center gap-3">
            <Button icon={<HomeOutlined />} onClick={() => navigate('/')}>
              返回首页
            </Button>
            <Button type="primary" icon={<PlusOutlined />} onClick={openCreate}>
              新建绘本
            </Button>
          </div>
        </div>

        <Tabs
          className="mt-4"
          activeKey={statusFilter}
          onChange={(key) => setStatusFilter(key as StatusFilter)}
          items={STATUS_TABS}
        />

        <Spin spinning={loading}>
          {books.length === 0 && !loading ? (
            <div className="py-20 flex justify-center">
              <Empty description="还没有绘本，点击下方按钮开始创作吧">
                <Button type="primary" icon={<PlusOutlined />} onClick={openCreate}>
                  新建绘本
                </Button>
              </Empty>
            </div>
          ) : (
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-5 mt-2">
              {books.map((book) => (
                <BookCard
                  key={book.id}
                  book={book}
                  onView={(item) => navigate(`/books/${item.id}`)}
                  onEdit={openEdit}
                />
              ))}
            </div>
          )}
        </Spin>

        {total > limit ? (
          <div className="flex justify-center mt-8">
            <Pagination
              current={page}
              pageSize={limit}
              total={total}
              showSizeChanger={false}
              onChange={(nextPage) => fetchBooks({ page: nextPage, status: queryStatus })}
            />
          </div>
        ) : null}
      </div>

      <BookFormModal
        open={modalOpen}
        book={editing}
        confirmLoading={saving}
        onCancel={closeModal}
        onSubmit={handleSubmit}
      />
    </div>
  )
}

export default BookList
