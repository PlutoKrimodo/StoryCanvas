export interface User {
  id: string
  username: string
  email: string
  avatar?: string | null
  created_at?: string
}

export interface LoginRequest {
  email: string
  password: string
  /** 登录方式：email（邮箱）或 username（用户名）；缺省兼容旧客户端 */
  login_type?: 'email' | 'username'
  /** 可选：游客身份，登录后认领游客期间生成的图片 */
  guest_id?: string
}

export interface RegisterRequest {
  username: string
  email: string
  password: string
  /** 可选：游客身份，注册后认领游客期间生成的图片 */
  guest_id?: string
}

/** 游客会话（Guest-first：未登录用户先体验生成） */
export interface GuestSession {
  guest_id: string
  guest_token: string
  /** 游客可免费生成的图片数（用于前端额度提示） */
  quota: number
}

export interface AuthResponse {
  user: User
  access_token: string
  refresh_token: string
}

/** 刷新接口可能不回传 user，故该字段可选 */
export interface RefreshResponse {
  access_token: string
  refresh_token: string
  user?: User
}
