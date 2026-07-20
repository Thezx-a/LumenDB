# Docker Containerization

> This tutorial uses Docker for deployment and distribution. This document covers Docker fundamentals.

## Core Concepts

| Concept | Description |
|---------|-------------|
| **Image** | A read-only template containing everything needed to run an application (code, runtime, libraries) |
| **Container** | A running instance of an image; an isolated process |
| **Dockerfile** | A script for building images |
| **Volume** | Persistent data storage |
| **Network** | Communication between containers |

## Dockerfile Basics

```dockerfile
# Multi-stage build example
FROM gcc-12 AS builder
WORKDIR /app
COPY . .
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

FROM ubuntu:22.04 AS runtime
COPY --from=builder /app/build/bin /usr/local/bin
EXPOSE 8080
CMD ["deepvector-server"]
```

### Common Instructions

| Instruction | Description |
|-------------|-------------|
| `FROM` | Base image |
| `WORKDIR` | Working directory |
| `COPY` | Copy files into the image |
| `RUN` | Execute a command |
| `EXPOSE` | Declare a port |
| `CMD` | Container startup command |

### Layer Caching Optimization

```dockerfile
# Good: copy dependency files first, then source code
COPY CMakeLists.txt .
RUN cmake -B build && cmake --build build  # This layer will be cached

COPY src/ src/
RUN cmake --build build -j$(nproc)  # Only rebuilds when source code changes
```

## docker-compose

```yaml
version: '3.8'
services:
  deepvector:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - data:/app/data
    environment:
      - API_KEY=my-secret-key

volumes:
  data:
```

### Common Commands

```bash
# Build and start
docker compose up -d

# View logs
docker compose logs -f

# Stop
docker compose down

# Enter container
docker compose exec deepvector bash
```

## Image Size Optimization

| Technique | Effect |
|-----------|--------|
| Multi-stage builds | Removes build toolchain, reduces size by 60-80% |
| Using Alpine base image | Smaller base image |
| Merging RUN instructions | Reduces layer count |
| Using .dockerignore | Excludes unnecessary files |

## Related Chapters

- Ch12 production deployment: [12_生产部署](../ch12_production/12_生产部署_zh.md)
