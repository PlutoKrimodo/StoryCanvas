import { cleanup, render, screen } from '@testing-library/react'
import { MemoryRouter, Route, Routes } from 'react-router-dom'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import type { Book } from '../../types/book'
import type { BookPage } from '../../types/page'
import BookCard from '../../components/book/BookCard'
import BookDetail from './BookDetail'
import BookList from './BookList'

const book: Book = {
  id: 'b1',
  user_id: 'u1',
  title: '雨后的森林',
  description: '小兔子第一次独自穿过森林',
  cover_image: null,
  status: 'draft',
  created_at: '2026-10-12T00:00:00.000Z',
  updated_at: '2026-10-12T00:00:00.000Z',
}

const pages: BookPage[] = [
  {
    id: 'p1',
    book_id: 'b1',
    page_number: 1,
    title: '雨后的相遇',
    image_url: 'https://cdn.example.com/p1.png',
  },
]

const bookStore = {
  books: [] as Book[],
  total: 0,
  page: 1,
  limit: 12,
  loading: false,
  current: null as Book | null,
  detailLoading: false,
  fetchBooks: vi.fn().mockResolvedValue(undefined),
  fetchBook: vi.fn().mockResolvedValue(undefined),
  createBook: vi.fn(),
  updateBook: vi.fn(),
  removeBook: vi.fn(),
}

vi.mock('../../stores/bookStore', () => ({ useBookStore: () => bookStore }))
vi.mock('../../api/pages', () => ({
  pagesApi: { list: () => Promise.resolve({ data: { data: pages } }) },
}))

describe('绘本域接入新主题（docs/17 Step 7）', () => {
  beforeEach(() => {
    bookStore.books = []
    bookStore.total = 0
    bookStore.loading = false
    bookStore.current = null
    bookStore.detailLoading = false
  })

  afterEach(() => cleanup())

  it('BookList：「新建绘本」进 PageHeader 操作区，空态是画布虚线面板', () => {
    const { container } = render(
      <MemoryRouter initialEntries={['/books']}>
        <BookList />
      </MemoryRouter>,
    )
    expect(screen.getByRole('heading', { name: '我的绘本' })).toBeInTheDocument()
    expect(screen.getAllByRole('button', { name: /新建绘本/ })).toHaveLength(2)
    expect(container.querySelector('.border-dashed')).not.toBeNull()
    expect(container.querySelector('.bg-gradient-to-br')).toBeNull()
  })

  it('BookCard：暖色封面占位 + shadow-lift 悬停，无冷色渐变', () => {
    const { container } = render(<BookCard book={book} onView={vi.fn()} onEdit={vi.fn()} />)
    const card = container.querySelector('.ant-card')
    expect(card?.className).toContain('hover:shadow-lift')
    expect(card?.querySelector('.bg-gradient-to-br')).toBeNull()
    expect(card?.querySelector('.bg-canvas2')).not.toBeNull()
    expect(screen.getByText('雨后的森林')).toBeInTheDocument()
  })

  it('BookDetail：头部汇总标题与操作，页图用 hairline 令牌', async () => {
    bookStore.current = book
    const { container } = render(
      <MemoryRouter initialEntries={['/books/b1']}>
        <Routes>
          <Route path="/books/:id" element={<BookDetail />} />
        </Routes>
      </MemoryRouter>,
    )

    // 先把页面列表的异步加载冲刷进 act，再断言，避免 setState 落在 act 之外
    expect(await screen.findByText(/雨后的相遇/)).toBeInTheDocument()
    expect(screen.getByRole('heading', { name: '雨后的森林' })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /编\s*辑/ })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /删\s*除/ })).toBeInTheDocument()

    expect(container.querySelector('.border-slate-100')).toBeNull()
    expect(container.querySelector('.border-hairline')).not.toBeNull()
    expect(screen.getByText('1')).toBeInTheDocument()
    // 有页面才允许导出 PDF（Step 7 的回归点：保存→导出链路）
    expect(screen.getByRole('button', { name: /导出 PDF/ })).toBeEnabled()
  })

  it('BookDetail：加载态用骨架屏而不是大 Spin', () => {
    bookStore.detailLoading = true
    const { container } = render(
      <MemoryRouter initialEntries={['/books/b1']}>
        <Routes>
          <Route path="/books/:id" element={<BookDetail />} />
        </Routes>
      </MemoryRouter>,
    )
    expect(container.querySelector('.ant-skeleton')).not.toBeNull()
    expect(container.querySelector('.ant-spin')).toBeNull()
  })
})
