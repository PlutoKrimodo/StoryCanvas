# ============================================================
# StoryCanvas AI Service 镜像
#
# 说明：
#   - 服务商密钥（OPENAI_API_KEY / STABILITY_API_KEY）与内部鉴权密钥
#     INTERNAL_API_KEY 一律通过环境变量注入，镜像内不含明文凭据
#   - 以非 root 用户运行
# ============================================================

FROM python:3.10-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1

WORKDIR /app

# 依赖单独分层，代码变更时可复用缓存
COPY requirements.txt ./
RUN pip install --upgrade pip && pip install -r requirements.txt

COPY . .

RUN useradd --create-home --shell /bin/bash appuser \
    && mkdir -p /app/storage \
    && chown -R appuser:appuser /app
USER appuser

EXPOSE 8000

CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]
