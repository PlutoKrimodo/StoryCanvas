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
}

export interface RegisterRequest {
  username: string
  email: string
  password: string
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
