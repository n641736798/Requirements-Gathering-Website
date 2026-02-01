import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  // 后端地址：同机部署用 localhost，前后端分离时在 .env 中设置 VITE_API_TARGET
  const API_TARGET = env.VITE_API_TARGET || 'http://localhost:8080'

  return {
    plugins: [react()],
    server: {
      port: 3000,
      proxy: {
        '/api': {
          target: API_TARGET,
          changeOrigin: true,
          timeout: 30000, // 30 秒，避免后端/数据库慢导致 504
        }
      }
    }
  }
})
