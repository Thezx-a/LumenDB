# Chapter 11: Docker & Deployment

> Containerized DeepVector + Agent production deployment.

## Prerequisites

> 📎 **Reference**: [Docker](../prerequisites/03_Docker容器化_en.md) | [Build Environment](../prerequisites/01_构建环境配置_en.md)

---

## Learning Objectives

- Understand multi-stage Docker builds
- Master docker-compose multi-service orchestration
- Learn production deployment checklist

---

## 11.1 Multi-Stage Build

```dockerfile
# Stage 1: Build
FROM ubuntu:22.04 AS builder
RUN apt-get install -y g++-12 cmake ninja-build
COPY . .
RUN cmake -B build && cmake --build build --target deepvector_server

# Stage 2: Runtime
FROM ubuntu:22.04
COPY --from=builder /build/build/deepvector_server /usr/local/bin/
EXPOSE 8080
CMD ["deepvector_server", "--port", "8080"]
```

---

## 11.2 Docker Compose

```yaml
services:
  deepvector:
    build:
      context: .
      target: deepvector-runtime
    ports: ["8080:8080"]
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      
  agent:
    build:
      context: .
      target: agent-runtime
    ports: ["8090:8090"]
    depends_on:
      deepvector:
        condition: service_healthy
    environment:
      - AGENTICDB_LLM_PROVIDER=${LLM_PROVIDER:-ollama}
```

---

## 11.3 Production Checklist

| Category | Item | Description |
|----------|------|-------------|
| Security | API Key | DeepVector `--api-key` protection |
| Security | HTTPS | Let's Encrypt reverse proxy |
| Reliability | Auto-restart | systemd/docker restart policy |
| Reliability | Health checks | Docker HEALTHCHECK |
| Monitoring | Logs | logrotate |
| Monitoring | Alerts | Search P99 > 500ms |
| Performance | Resource limits | CPU/Memory cgroups |
| Data | Backup | Regular data dir backups |

---

## Review Questions

1. In multi-stage builds, why use `ubuntu:22.04` for builder instead of `python:3.11`?
2. What's the `profiles` parameter in docker-compose for? When to use "full"?
3. How to achieve zero-downtime deployment with two rolling containers?

## Hands-on Exercises

1. Add health check to ensure agent waits for deepvector
2. Create a `.env.example` with all available env vars
3. Write a `docker-compose.prod.yml` with resource limits and log config
