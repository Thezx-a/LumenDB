FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential g++-12 cmake git libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY CMakeLists.txt /app/
COPY src/ /app/src/
WORKDIR /app

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target lumendb -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash lumendb
USER lumendb

COPY --from=builder /app/build/lumendb /usr/local/bin/lumendb
COPY deepvector.json /etc/lumendb/config.json

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/lumendb"]
CMD ["--config", "/etc/lumendb/config.json"]
