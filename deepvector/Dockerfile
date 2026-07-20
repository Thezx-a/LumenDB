# =============================================================================
# Stage 1: 鏋勫缓 DeepVector C++ Server / Build DeepVector C++ Server
# =============================================================================
FROM ubuntu:22.04 AS builder

# 瀹夎鏋勫缓宸ュ叿 / Install build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-12 cmake ninja-build ca-certificates git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# 鍏堟嫹璐?CMake 鍜屼緷璧栧畾涔? 鍒╃敤 Docker 缂撳瓨 / Leverage Docker cache for deps
COPY CMakeLists.txt ./
COPY vendor/ ./vendor/

# 鍐嶆嫹璐濇簮鐮?/ Copy source code
COPY src/ ./src/
COPY include/ ./include/

# Release 鏋勫缓, 鍚敤 AVX2 SIMD 鍔犻€?/ Release build with AVX2
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-12 \
    -DUSE_AVX2=ON
RUN cmake --build build -j$(nproc) --target lumendb_server

# =============================================================================
# Stage 2: DeepVector 杩愯闀滃儚 / DeepVector Runtime Image
# =============================================================================
FROM ubuntu:22.04 AS lumendb-runtime

# 浠呭畨瑁呰繍琛屾椂渚濊禆 / Minimal runtime deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -s /bin/bash lumendb

# 浠庢瀯寤洪樁娈垫嫹璐濅簩杩涘埗 / Copy binary from builder
COPY --from=builder /build/build/server/lumendb_server /usr/local/bin/lumendb_server

# 鍒涘缓鏁版嵁鐩綍 / Create data directory
RUN mkdir -p /data && chown lumendb:lumendb /data

# 鍋ュ悍妫€鏌?/ Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# 杩愯閰嶇疆 / Runtime config
USER lumendb
EXPOSE 8080
VOLUME /data

ENV DEEPVECTOR_DIM=384
ENV DEEPVECTOR_PORT=8080
ENV DEEPVECTOR_DATA_DIR=/data
ENV DEEPVECTOR_API_KEY=""

ENTRYPOINT ["/usr/local/bin/lumendb_server"]
CMD ["--host", "0.0.0.0", "--port", "8080", "--dim", "384", "--data-dir", "/data"]

# =============================================================================
# Stage 3: Agent Server 杩愯闀滃儚 / Agent Server Runtime Image
# =============================================================================
FROM python:3.11-slim AS agent-runtime

# 瀹夎 Python 渚濊禆 / Install Python dependencies
RUN pip install --no-cache-dir \
    httpx pydantic sentence-transformers \
    fastapi uvicorn mcp

# 鎷疯礉 Agent 浠ｇ爜 / Copy agent code
WORKDIR /app
COPY agent/ ./agent/
COPY scripts/ ./scripts/
COPY examples/ ./examples/

# 闈?root 鐢ㄦ埛杩愯 / Run as non-root
RUN useradd -m -s /bin/bash agentuser
USER agentuser

EXPOSE 8090

ENV AGENTICDB_DEEPVECTOR_URL="http://localhost:8080"
ENV AGENTICDB_AGENT_PORT=8090
ENV AGENTICDB_LLM_PROVIDER="ollama"
ENV AGENTICDB_EMBEDDING_PROVIDER="local"

ENTRYPOINT ["python", "-m", "agent.server.app"]
