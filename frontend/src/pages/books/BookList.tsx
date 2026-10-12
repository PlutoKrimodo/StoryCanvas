import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { App, Button, Empty, Pagination, Spin, Tabs } from 'antd'
import { PlusOutlined } from '@ant-design/icons'

import AppLayout from '../../components/layout/AppLayout'
import BookCard from '../../components/book/BookCard'
import BookFormModal, { type BookFormValues } from '../../components/book/BookFormModal'
import PageHeader from '../../components/layout/PageHeader'
import { useBookStore } from '../../stores/bookStore'
import type { Book, BookStatus, CreateBookRequest } from '../../types/book'
import { BOOK_STATUS_OPTIONS } from '../../utils/constants'

type StatusFilter = BookStatus | 'all'

const STATUS_TABS = [
  { key: 'all', label: '全部' },
  ...BOOK_STATUS_OPTIONS.map((option) => ({ key: option.value, label: option.label })),
]

function BookList() {
  const navigate = useNavigate()
  const { message } = App.useApp()
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
    <AppLayout variant="app">
      <PageHeader
        title="我的绘本"
        description="用一段故事文字，生成属于你的儿童绘本插画"
        extra={
          <Button type="primary" icon={<PlusOutlined />} onClick={openCreate}>
            新建绘本
          </Button>
        }
      />

      <Tabs activeKey={statusFilter} onChange={(key) => setStatusFilter(key as StatusFilter)} items={STATUS_TABS} />

      <Spin spinning={loading}>
        {books.length === 0 && !loading ? (
          <div className="mt-2 flex justify-center rounded-card border border-dashed border-hairline bg-canvas2 py-16">
            <Empty description="还没有绘本，从「新建绘本」开始创作">
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

      <BookFormModal
        open={modalOpen}
        book={editing}
        confirmLoading={saving}
        onCancel={closeModal}
        onSubmit={handleSubmit}
      />
    </AppLayout>
  )
}

export default BookList
