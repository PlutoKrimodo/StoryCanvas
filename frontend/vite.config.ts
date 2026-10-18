import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    // 监听所有网卡，供同一局域网的其他设备访问（WSL 下配合 Windows 端口转发）
    host: '0.0.0.0',
    port: 5173,
    // Vite 5.4.12+ 的 Host 校验：局域网 IP / 自定义域名访问时放行
    allowedHosts: true,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
})
