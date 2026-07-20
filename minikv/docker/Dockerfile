FROM gcc:12 AS builder
WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12 && cmake --build build -j

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /build/build/minikv_server /usr/local/bin/
RUN mkdir -p /data/minikv
EXPOSE 8888
CMD ["minikv_server", "--host", "0.0.0.0", "--port", "8888", "--db", "/data/minikv"]
