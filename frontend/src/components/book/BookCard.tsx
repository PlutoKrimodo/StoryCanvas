import { Button, Card, Tag, Typography } from 'antd'
import { BookOutlined, EditOutlined, EyeOutlined } from '@ant-design/icons'

import type { Book } from '../../types/book'
import { BOOK_STATUS_COLOR, BOOK_STATUS_TEXT } from '../../utils/constants'
import { formatDateTime } from '../../utils/datetime'

const { Paragraph, Text } = Typography

interface BookCardProps {
  book: Book
  onView: (book: Book) => void
  onEdit: (book: Book) => void
}

/** 绘本卡片：封面占位 + 标题 + 状态标签 + 描述摘要 + 操作入口 */
function BookCard({ book, onView, onEdit }: BookCardProps) {
  return (
    <Card
      hoverable
      bodyStyle={{ padding: 0 }}
      className="h-full !rounded-2xl overflow-hidden transition-all duration-300 hover:-translate-y-1 hover:shadow-xl"
      onClick={() => onView(book)}
    >
      <div className="h-40 bg-gradient-to-br from-violet-400 via-indigo-400 to-sky-400 flex items-center justify-center">
        {book.cover_image ? (
          <img src={book.cover_image} alt={book.title} className="h-full w-full object-cover" />
        ) : (
          <BookOutlined className="text-5xl text-white/90" />
        )}
      </div>

      <div className="p-4 flex flex-col gap-2">
        <div className="flex items-start justify-between gap-2">
          <Text strong className="text-base truncate">
            {book.title}
          </Text>
          <Tag color={BOOK_STATUS_COLOR[book.status]} className="!m-0 !mr-0 shrink-0">
            {BOOK_STATUS_TEXT[book.status]}
          </Tag>
        </div>

        <Paragraph type="secondary" className="!mb-0 text-sm line-clamp-2 min-h-[40px]">
          {book.description || '暂无描述'}
        </Paragraph>

        <div className="flex items-center justify-between mt-1">
          <Text type="secondary" className="text-xs">
            {formatDateTime(book.created_at, 'YYYY-MM-DD')}
          </Text>
          <div className="flex gap-1">
            <Button
              size="small"
              type="text"
              icon={<EyeOutlined />}
              onClick={(event) => {
                event.stopPropagation()
                onView(book)
              }}
            >
              查看
            </Button>
            <Button
              size="small"
              type="text"
              icon={<EditOutlined />}
              onClick={(event) => {
                event.stopPropagation()
                onEdit(book)
              }}
            >
              编辑
            </Button>
          </div>
        </div>
      </div>
    </Card>
  )
}

export default BookCard
