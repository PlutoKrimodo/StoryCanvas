# ============================================================
# StoryCanvas 前端镜像
#
# 构建阶段用 Node 产出静态资源，运行阶段用 Nginx 托管并反代 /api 到后端
# ============================================================

FROM node:18-alpine AS builder

WORKDIR /app

# package-lock.json 可选拷贝：锁文件与 package.json 不一致时 npm ci 会失败，
# 这里用 npm install 以获得更好的容错性
COPY package.json package-lock.json* ./
RUN npm install

COPY . .
RUN npm run build

# ==================== 运行阶段 ====================
FROM nginx:alpine

COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
