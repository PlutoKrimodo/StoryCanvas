import { useEffect, useState } from 'react'
import { Alert, Form, Input, InputNumber, Modal, Select, message } from 'antd'

import { booksApi } from '../../api/books'
import { pagesApi } from '../../api/pages'
import type { Book } from '../../types/book'
import { MAX_PAGES_PER_BOOK } from '../../utils/constants'

interface SaveToBookModalProps {
  open: boolean
  imageId: string | null
  /** 默认选中的绘本（从绘本详情页进入时传入） */
  defaultBookId?: string
  onCancel: () => void
  onSaved?: (bookId: string) => void
}

interface SaveToBookFormValues {
  book_id: string
  page_number: number
  title?: string
}

/**
 * 「保存到绘本」弹窗：
 * 选择目标绘本 + 页码 + 页标题，把当前生成图绑定为绘本的一页（用于 PDF 导出）。
 */
function SaveToBookModal({
  open,
  imageId,
  defaultBookId,
  onCancel,
  onSaved,
}: SaveToBookModalProps) {
  const [form] = Form.useForm<SaveToBookFormValues>()
  const [books, setBooks] = useState<Book[]>([])
  const [loadingBooks, setLoadingBooks] = useState(false)
  const [saving, setSaving] = useState(false)

  const selectedBookId = Form.useWatch('book_id', form)

  // 打开时加载用户的绘本列表
  useEffect(() => {
    if (!open) {
      return
    }
    setLoadingBooks(true)
    booksApi
      .listMine({ page: 1, limit: 100 })
      .then(({ data }) => {
        const items = data.data?.items ?? []
        setBooks(items)
        const initial =
          defaultBookId && items.some((book) => book.id === defaultBookId)
            ? defaultBookId
            : items[0]?.id
        form.setFieldsValue({ book_id: initial, page_number: 1, title: '' })
      })
      .catch((error) => console.error('加载绘本列表失败', error))
      .finally(() => setLoadingBooks(false))
  }, [open, defaultBookId, form])

  // 切换绘本时自动推荐下一页页码
  useEffect(() => {
    if (!open || !selectedBookId) {
      return
    }
    pagesApi
      .list(selectedBookId)
      .then(({ data }) => {
        const pages = data.data ?? []
        const next = pages.length
          ? Math.max(...pages.map((page) => page.page_number)) + 1
          : 1
        form.setFieldValue('page_number', Math.min(next, MAX_PAGES_PER_BOOK))
      })
      .catch((error) => console.error('加载绘本页失败', error))
  }, [open, selectedBookId, form])

  const handleOk = () => {
    if (!imageId) {
      message.warning('当前没有可保存的生成图')
      return
    }
    form
      .validateFields()
      .then(async (values) => {
        setSaving(true)
        try {
          await pagesApi.save(values.book_id, {
            page_number: values.page_number,
            title: values.title?.trim() || undefined,
            image_id: imageId,
          })
          message.success('已保存到绘本')
          onSaved?.(values.book_id)
          onCancel()
        } catch (error) {
          console.error('保存到绘本失败', error)
        } finally {
          setSaving(false)
        }
      })
      .catch(() => undefined)
  }

  return (
    <Modal
      title="保存到绘本"
      open={open}
      onOk={handleOk}
      onCancel={onCancel}
      confirmLoading={saving}
      okText="保存"
      cancelText="取消"
      okButtonProps={{ disabled: books.length === 0 }}
      destroyOnClose
    >
      {books.length === 0 && !loadingBooks ? (
        <Alert
          type="warning"
          showIcon
          className="mb-4"
          message="你还没有绘本"
          description="请先在「我的绘本」中创建一本绘本，再回来保存这张插画。"
        />
      ) : null}

      <Form form={form} layout="vertical" requiredMark={false} className="mt-2">
        <Form.Item
          name="book_id"
          label="目标绘本"
          rules={[{ required: true, message: '请选择绘本' }]}
        >
          <Select
            loading={loadingBooks}
            placeholder="选择绘本"
            options={books.map((book) => ({ value: book.id, label: book.title }))}
          />
        </Form.Item>

        <Form.Item
          name="page_number"
          label="页码"
          rules={[{ required: true, message: '请输入页码' }]}
        >
          <InputNumber min={1} max={MAX_PAGES_PER_BOOK} style={{ width: '100%' }} />
        </Form.Item>

        <Form.Item name="title" label="页标题（可选，导出 PDF 时显示）">
          <Input placeholder="例如：雨后的相遇" maxLength={200} />
        </Form.Item>
      </Form>
    </Modal>
  )
}

export default SaveToBookModal
