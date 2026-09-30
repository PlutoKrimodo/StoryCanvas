import { useEffect, type ReactNode } from 'react'
import { BrowserRouter, Navigate, Route, Routes } from 'react-router-dom'

import ErrorBoundary from './components/common/ErrorBoundary'
import { ensureGuestToken } from './api/client'
import BookDetail from './pages/books/BookDetail'
import BookList from './pages/books/BookList'
import Generate from './pages/generation/Generate'
import Home from './pages/home/Home'
import Login from './pages/auth/Login'
import Register from './pages/auth/Register'
import Profile from './pages/profile/Profile'
import { useAuthStore } from './stores/authStore'

const ProtectedRoute = ({ children }: { children: ReactNode }) => {
  const isAuthenticated = useAuthStore((state) => state.isAuthenticated)
  return isAuthenticated ? <>{children}</> : <Navigate to="/login" replace />
}

function App() {
  const isAuthenticated = useAuthStore((state) => state.isAuthenticated)

  // Guest-first：未登录时静默创建游客会话，使首页生成器无需登录即可调用受保护接口
  useEffect(() => {
    if (!isAuthenticated) {
      void ensureGuestToken()
    }
  }, [isAuthenticated])

  return (
    <ErrorBoundary>
      <BrowserRouter>
        <Routes>
          {/* 公开路由 */}
          <Route path="/login" element={<Login />} />
          <Route path="/register" element={<Register />} />

          {/* Guest-first：首页与生成页对游客开放（先体验后注册） */}
          <Route path="/" element={<Home />} />
          <Route path="/generate" element={<Generate />} />

          {/* 受保护路由 */}
          <Route
            path="/profile"
            element={
              <ProtectedRoute>
                <Profile />
              </ProtectedRoute>
            }
          />
          <Route
            path="/books"
            element={
              <ProtectedRoute>
                <BookList />
              </ProtectedRoute>
            }
          />
          <Route
            path="/books/:id"
            element={
              <ProtectedRoute>
                <BookDetail />
              </ProtectedRoute>
            }
          />

          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </BrowserRouter>
    </ErrorBoundary>
  )
}

export default App
