FROM gcc:12 AS builder
WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12
RUN cmake --build build -j4

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /build/build/skynet_gateway /usr/local/bin/
COPY gateway/gateway.yaml /etc/skynet/gateway.yaml
EXPOSE 8080
CMD ["skynet_gateway", "--config", "/etc/skynet/gateway.yaml"]
