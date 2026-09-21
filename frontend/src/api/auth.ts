import client from './client'
import type { ApiResponse } from '../types/api'
import type { AuthResponse, LoginRequest, RegisterRequest, User } from '../types/auth'

export interface UpdateUserRequest {
  username?: string
  avatar?: string
}

export interface ChangePasswordRequest {
  old_password: string
  new_password: string
}

export const authApi = {
  register: (payload: RegisterRequest) =>
    client.post<ApiResponse<AuthResponse>>('/auth/register', payload),

  login: (payload: LoginRequest) =>
    client.post<ApiResponse<AuthResponse>>('/auth/login', payload),

  logout: () => client.post<ApiResponse<null>>('/auth/logout'),

  getMe: () => client.get<ApiResponse<User>>('/users/me'),

  updateMe: (payload: UpdateUserRequest) =>
    client.put<ApiResponse<User>>('/users/me', payload),

  changePassword: (payload: ChangePasswordRequest) =>
    client.put<ApiResponse<null>>('/users/me/password', payload),
}
