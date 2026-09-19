# ============================================================
# StoryCanvas C++ 后端镜像
#
# 说明：
#   - 构建与运行均使用官方 Drogon 镜像，保证 Drogon / OpenSSL 等共享库版本一致
#   - 若需要 bcrypt 密码哈希，可在构建阶段额外安装 libbcrypt；
#     未安装时应用自动回退为 PBKDF2-HMAC-SHA256（见 backend/CMakeLists.txt）
#   - 密钥类配置一律通过环境变量注入（见 docker-compose.yml），镜像内不含任何明文凭据
# ============================================================

FROM drogonframework/drogon:latest AS builder

WORKDIR /src

# 先拷贝构建描述与源码，利用 Docker 层缓存
COPY CMakeLists.txt ./
COPY main.cc ./
COPY config ./config
COPY controllers ./controllers
COPY dto ./dto
COPY filters ./filters
COPY utils ./utils
COPY services ./services
COPY models ./models
COPY repositories ./repositories

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

# ==================== 运行阶段 ====================
FROM drogonframework/drogon:latest

WORKDIR /app

RUN mkdir -p /app/logs /app/storage

COPY --from=builder /src/build/storycanvas_backend /app/storycanvas_backend
COPY --from=builder /src/config /app/config

EXPOSE 8080

CMD ["./storycanvas_backend"]
