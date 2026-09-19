import client from './client'
import type { ApiResponse } from '../types/api'
import type { AuthResponse, LoginRequest, RegisterRequest } from '../types/auth'

export const authApi = {
  register: (payload: RegisterRequest) =>
    client.post<ApiResponse<AuthResponse>>('/auth/register', payload),

  login: (payload: LoginRequest) =>
    client.post<ApiResponse<AuthResponse>>('/auth/login', payload),

  logout: () => client.post<ApiResponse<null>>('/auth/logout'),
}
