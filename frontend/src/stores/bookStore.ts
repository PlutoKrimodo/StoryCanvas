import { create } from 'zustand'

import { booksApi } from '../api/books'
import type { Book, BookStatus, CreateBookRequest, UpdateBookRequest } from '../types/book'

const DEFAULT_LIMIT = 9

interface FetchBooksParams {
  page?: number
  limit?: number
  status?: BookStatus
}

interface BookState {
  books: Book[]
  total: number
  page: number
  limit: number
  status?: BookStatus
  loading: boolean

  current: Book | null
  detailLoading: boolean

  fetchBooks: (params?: FetchBooksParams) => Promise<void>
  fetchBook: (bookId: string) => Promise<Book | null>
  createBook: (payload: CreateBookRequest) => Promise<Book | null>
  updateBook: (bookId: string, payload: UpdateBookRequest) => Promise<Book | null>
  removeBook: (bookId: string) => Promise<boolean>
}

/**
 * 绘本列表 / 详情状态。
 * 错误提示统一由 axios 响应拦截器负责，这里仅记录日志并复位 loading。
 */
export const useBookStore = create<BookState>((set, get) => ({
  books: [],
  total: 0,
  page: 1,
  limit: DEFAULT_LIMIT,
  status: undefined,
  loading: false,

  current: null,
  detailLoading: false,

  fetchBooks: (params) => {
    const page = params?.page ?? 1
    const limit = params?.limit ?? get().limit
    const status = params?.status
    set({ loading: true })
    return booksApi
      .listMine({ page, limit, status })
      .then(({ data }) => {
        set({
          books: data.data?.items ?? [],
          total: data.data?.total ?? 0,
          page: data.data?.page ?? page,
          limit: data.data?.limit ?? limit,
          status,
        })
      })
      .catch((error) => {
        console.error('加载绘本列表失败', error)
      })
      .finally(() => set({ loading: false }))
  },

  fetchBook: (bookId) => {
    set({ detailLoading: true, current: null })
    return booksApi
      .get(bookId)
      .then(({ data }) => {
        set({ current: data.data })
        return data.data
      })
      .catch((error) => {
        console.error('加载绘本详情失败', error)
        set({ current: null })
        return null
      })
      .finally(() => set({ detailLoading: false }))
  },

  createBook: (payload) =>
    booksApi
      .create(payload)
      .then(({ data }) => data.data)
      .catch((error) => {
        console.error('创建绘本失败', error)
        return null
      }),

  updateBook: (bookId, payload) =>
    booksApi
      .update(bookId, payload)
      .then(({ data }) => {
        set({ current: data.data })
        return data.data
      })
      .catch((error) => {
        console.error('更新绘本失败', error)
        return null
      }),

  removeBook: (bookId) =>
    booksApi
      .remove(bookId)
      .then(() => {
        if (get().current?.id === bookId) {
          set({ current: null })
        }
        return true
      })
      .catch((error) => {
        console.error('删除绘本失败', error)
        return false
      }),
}))
