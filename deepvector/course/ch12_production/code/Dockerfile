FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential g++-12 cmake git libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY CMakeLists.txt /app/
COPY src/ /app/src/
WORKDIR /app

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target deepvector -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash deepvector
USER deepvector

COPY --from=builder /app/build/deepvector /usr/local/bin/deepvector
COPY deepvector.json /etc/deepvector/config.json

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/deepvector"]
CMD ["--config", "/etc/deepvector/config.json"]
