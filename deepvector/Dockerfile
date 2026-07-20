# DeepVector monorepo-aware Dockerfile
# Build from repository root:
#   docker build -f deepvector/Dockerfile -t deepvector:local .
#
# Or from deepvector/ with parent context (see RUN.md).

# =============================================================================
# Stage 1: Build DeepVector C++ Server
# =============================================================================
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-12 cmake ninja-build ca-certificates git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
# Build context should be the monorepo root (hellocpp/)
COPY CMakeLists.txt ./
COPY minikv/ ./minikv/
COPY skynet/ ./skynet/
COPY deepvector/CMakeLists.txt ./deepvector/
COPY deepvector/include/ ./deepvector/include/
COPY deepvector/src/ ./deepvector/src/
COPY deepvector/tests/ ./deepvector/tests/
COPY deepvector/benchmarks/ ./deepvector/benchmarks/
COPY deepvector/python/ ./deepvector/python/

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-12 \
    -DUSE_AVX2=ON \
    -DENABLE_TESTS=OFF
RUN cmake --build build -j$(nproc) --target deepvector_server

# =============================================================================
# Stage 2: DeepVector Runtime
# =============================================================================
FROM ubuntu:22.04 AS deepvector-runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -s /bin/bash deepvector

COPY --from=builder /src/build/deepvector/deepvector_server /usr/local/bin/deepvector_server

RUN mkdir -p /data && chown deepvector:deepvector /data

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

USER deepvector
EXPOSE 8080
VOLUME /data

ENTRYPOINT ["/usr/local/bin/deepvector_server"]
CMD ["--host", "0.0.0.0", "--port", "8080", "--dim", "384", "--data-dir", "/data"]

# =============================================================================
# Stage 3: Agent Server Runtime
# =============================================================================
FROM python:3.11-slim AS agent-runtime

WORKDIR /app
COPY deepvector/requirements.txt ./requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

COPY deepvector/agent/ ./agent/
COPY deepvector/scripts/ ./scripts/

RUN useradd -m -s /bin/bash agentuser
USER agentuser

EXPOSE 8090

ENV AGENTICDB_DEEPVECTOR_URL="http://deepvector:8080"
ENV AGENTICDB_AGENT_PORT=8090
ENV AGENTICDB_LLM_PROVIDER="ollama"
ENV AGENTICDB_EMBEDDING_PROVIDER="local"

ENTRYPOINT ["python", "-m", "agent.server.app"]
