import { useEffect, useState } from 'react'
import { App, Modal, Select, Typography } from 'antd'

import { booksApi } from '../../api/books'
import { pagesApi } from '../../api/pages'
import type { Book } from '../../types/book'

const { Text } = Typography

interface SaveBatchModalProps {
  open: boolean
  imageIds: string[]
  defaultBookId?: string
  onCancel: () => void
  onSaved?: (bookId: string) => void
}

/** 把一组图片按顺序写入同一本绘本的连续页。 */
function SaveBatchModal({
  open,
  imageIds,
  defaultBookId,
  onCancel,
  onSaved,
}: SaveBatchModalProps) {
  const { message } = App.useApp()
  const [books, setBooks] = useState<Book[]>([])
  const [bookId, setBookId] = useState<string | undefined>(defaultBookId)
  const [loading, setLoading] = useState(false)
  const [saving, setSaving] = useState(false)

  useEffect(() => {
    if (!open) return
    setBookId(defaultBookId)
    setLoading(true)
    booksApi
      .listMine({ page: 1, limit: 100 })
      .then(({ data }) => {
        const items = data.data?.items ?? []
        setBooks(items)
        if (!defaultBookId && items.length === 1) {
          setBookId(items[0].id)
        }
      })
      .catch((error) => console.error('加载绘本列表失败', error))
      .finally(() => setLoading(false))
  }, [defaultBookId, open])

  const handleSave = async () => {
    if (!bookId || imageIds.length === 0) {
      message.warning('请选择绘本')
      return
    }
    setSaving(true)
    try {
      await pagesApi.saveBatch(bookId, imageIds)
      message.success(`已把 ${imageIds.length} 张图加入绘本`)
      onSaved?.(bookId)
      onCancel()
    } catch (error) {
      console.error('整组加入绘本失败', error)
      message.error('加入绘本失败，请稍后重试')
    } finally {
      setSaving(false)
    }
  }

  return (
    <Modal
      title={`把 ${imageIds.length} 张图加入绘本`}
      open={open}
      onCancel={onCancel}
      onOk={() => void handleSave()}
      okText="按顺序加入"
      cancelText="取消"
      confirmLoading={saving}
      okButtonProps={{ disabled: !bookId }}
    >
      <Text type="secondary" className="mb-2 block text-xs">
        图片将按照分镜顺序，连续追加到所选绘本的末尾页。
      </Text>
      <Select
        className="w-full"
        loading={loading}
        value={bookId}
        onChange={setBookId}
        placeholder="选择目标绘本"
        options={books.map((book) => ({ value: book.id, label: book.title }))}
      />
    </Modal>
  )
}

export default SaveBatchModal
