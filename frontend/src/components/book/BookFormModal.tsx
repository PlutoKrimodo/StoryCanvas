import { useEffect } from 'react'
import { Form, Input, Modal, Select } from 'antd'

import type { Book, BookStatus } from '../../types/book'
import { BOOK_STATUS_OPTIONS } from '../../utils/constants'

export interface BookFormValues {
  title: string
  description?: string
  status: BookStatus
}

interface BookFormModalProps {
  open: boolean
  /** 传入绘本表示编辑，null 表示新建 */
  book: Book | null
  confirmLoading?: boolean
  onCancel: () => void
  onSubmit: (values: BookFormValues) => void
}

/** 新建 / 编辑绘本的公共表单弹窗（列表页与详情页复用） */
function BookFormModal({ open, book, confirmLoading, onCancel, onSubmit }: BookFormModalProps) {
  const [form] = Form.useForm<BookFormValues>()

  useEffect(() => {
    if (!open) {
      return
    }
    if (book) {
      form.setFieldsValue({
        title: book.title,
        description: book.description ?? '',
        status: book.status,
      })
    } else {
      form.resetFields()
      form.setFieldsValue({ status: 'draft' })
    }
  }, [open, book, form])

  const handleOk = () => {
    form
      .validateFields()
      .then((values) => onSubmit(values))
      .catch((error) => {
        // 校验失败返回的是表单校验信息对象，仅记录真正的异常
        if (error instanceof Error) {
          console.error('绘本表单校验异常', error)
        }
      })
  }

  return (
    <Modal
      title={book ? '编辑绘本' : '新建绘本'}
      open={open}
      onOk={handleOk}
      onCancel={onCancel}
      confirmLoading={confirmLoading}
      okText={book ? '保存' : '创建'}
      cancelText="取消"
      destroyOnClose
    >
      <Form form={form} layout="vertical" requiredMark={false} className="mt-4">
        <Form.Item
          name="title"
          label="标题"
          rules={[
            { required: true, message: '请输入绘本标题' },
            { max: 200, message: '标题不能超过 200 个字符' },
          ]}
        >
          <Input placeholder="例如：小红帽的故事" maxLength={200} allowClear />
        </Form.Item>

        <Form.Item
          name="description"
          label="描述"
          rules={[{ max: 2000, message: '描述不能超过 2000 个字符' }]}
        >
          <Input.TextArea
            rows={4}
            placeholder="简要描述这本绘本将要讲述的故事"
            maxLength={2000}
            showCount
          />
        </Form.Item>

        <Form.Item name="status" label="状态" rules={[{ required: true, message: '请选择状态' }]}>
          <Select options={BOOK_STATUS_OPTIONS} />
        </Form.Item>
      </Form>
    </Modal>
  )
}

export default BookFormModal
