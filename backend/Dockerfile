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

# Drogon 基础镜像只带框架自身依赖；本项目额外依赖 nlohmann_json / spdlog / libpq
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config \
    nlohmann-json3-dev libspdlog-dev libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# 先拷贝构建描述与源码，利用 Docker 层缓存
COPY CMakeLists.txt ./
COPY main.cpp ./
COPY config ./config
COPY controllers ./controllers
COPY dto ./dto
COPY filters ./filters
COPY utils ./utils
COPY services ./services
COPY models ./models
COPY repositories ./repositories
# CMakeLists.txt 无条件 add_subdirectory(tests)；缺少该目录会直接配置失败。
# tests/CMakeLists.txt 在未安装 GoogleTest 时会自动 return()，不会拖慢主程序构建。
COPY tests ./tests

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

# ==================== 运行阶段 ====================
FROM drogonframework/drogon:latest

WORKDIR /app

# 运行期需要 spdlog 与 libpq 的共享库（构建期安装的 -dev 包不会带入本阶段）
RUN apt-get update && apt-get install -y --no-install-recommends \
    libspdlog-dev libpq5 \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /app/logs /app/storage

COPY --from=builder /src/build/storycanvas_backend /app/storycanvas_backend
COPY --from=builder /src/config /app/config

EXPOSE 8080

CMD ["./storycanvas_backend"]
