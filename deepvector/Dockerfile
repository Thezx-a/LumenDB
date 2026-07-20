# =============================================================================
# Stage 1: 构建 DeepVector C++ Server / Build DeepVector C++ Server
# =============================================================================
FROM ubuntu:22.04 AS builder

# 安装构建工具 / Install build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-12 cmake ninja-build ca-certificates git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# 先拷贝 CMake 和依赖定义, 利用 Docker 缓存 / Leverage Docker cache for deps
COPY CMakeLists.txt ./
COPY vendor/ ./vendor/

# 再拷贝源码 / Copy source code
COPY src/ ./src/
COPY include/ ./include/

# Release 构建, 启用 AVX2 SIMD 加速 / Release build with AVX2
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-12 \
    -DUSE_AVX2=ON
RUN cmake --build build -j$(nproc) --target deepvector_server

# =============================================================================
# Stage 2: DeepVector 运行镜像 / DeepVector Runtime Image
# =============================================================================
FROM ubuntu:22.04 AS deepvector-runtime

# 仅安装运行时依赖 / Minimal runtime deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -s /bin/bash deepvector

# 从构建阶段拷贝二进制 / Copy binary from builder
COPY --from=builder /build/build/server/deepvector_server /usr/local/bin/deepvector_server

# 创建数据目录 / Create data directory
RUN mkdir -p /data && chown deepvector:deepvector /data

# 健康检查 / Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# 运行配置 / Runtime config
USER deepvector
EXPOSE 8080
VOLUME /data

ENV DEEPVECTOR_DIM=384
ENV DEEPVECTOR_PORT=8080
ENV DEEPVECTOR_DATA_DIR=/data
ENV DEEPVECTOR_API_KEY=""

ENTRYPOINT ["/usr/local/bin/deepvector_server"]
CMD ["--host", "0.0.0.0", "--port", "8080", "--dim", "384", "--data-dir", "/data"]

# =============================================================================
# Stage 3: Agent Server 运行镜像 / Agent Server Runtime Image
# =============================================================================
FROM python:3.11-slim AS agent-runtime

# 安装 Python 依赖 / Install Python dependencies
RUN pip install --no-cache-dir \
    httpx pydantic sentence-transformers \
    fastapi uvicorn mcp

# 拷贝 Agent 代码 / Copy agent code
WORKDIR /app
COPY agent/ ./agent/
COPY scripts/ ./scripts/
COPY examples/ ./examples/

# 非 root 用户运行 / Run as non-root
RUN useradd -m -s /bin/bash agentuser
USER agentuser

EXPOSE 8090

ENV AGENTICDB_DEEPVECTOR_URL="http://localhost:8080"
ENV AGENTICDB_AGENT_PORT=8090
ENV AGENTICDB_LLM_PROVIDER="ollama"
ENV AGENTICDB_EMBEDDING_PROVIDER="local"

ENTRYPOINT ["python", "-m", "agent.server.app"]
