# Docker 容器化

> 本教程使用 Docker 进行部署和分发。本文档覆盖 Docker 基础知识。

## 核心概念

| 概念 | 说明 |
|------|------|
| **镜像 (Image)** | 只读模板，包含运行应用所需的一切（代码、运行时、库） |
| **容器 (Container)** | 镜像的运行实例，隔离的进程 |
| **Dockerfile** | 构建镜像的脚本 |
| **卷 (Volume)** | 持久化数据存储 |
| **网络 (Network)** | 容器间通信 |

## Dockerfile 基础

```dockerfile
# 多阶段构建示例
FROM gcc-12 AS builder
WORKDIR /app
COPY . .
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

FROM ubuntu:22.04 AS runtime
COPY --from=builder /app/build/bin /usr/local/bin
EXPOSE 8080
CMD ["lumendb-server"]
```

### 常用指令

| 指令 | 说明 |
|------|------|
| `FROM` | 基础镜像 |
| `WORKDIR` | 工作目录 |
| `COPY` | 复制文件到镜像 |
| `RUN` | 执行命令 |
| `EXPOSE` | 声明端口 |
| `CMD` | 容器启动命令 |

### 层缓存优化

```dockerfile
# 好：先复制依赖文件，再复制源码
COPY CMakeLists.txt .
RUN cmake -B build && cmake --build build  # 这层会缓存

COPY src/ src/
RUN cmake --build build -j$(nproc)  # 只有源码变了才重新构建
```

## docker-compose

```yaml
version: '3.8'
services:
  lumendb:
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

### 常用命令

```bash
# 构建并启动
docker compose up -d

# 查看日志
docker compose logs -f

# 停止
docker compose down

# 进入容器
docker compose exec lumendb bash
```

## 镜像体积优化

| 技巧 | 效果 |
|------|------|
| 多阶段构建 | 去除编译工具链，减少 60-80% |
| 使用 Alpine 基础镜像 | 更小的 base image |
| 合并 RUN 指令 | 减少层数 |
| 使用 .dockerignore | 排除不需要的文件 |

## 相关章节

- Ch12 生产部署：[12_生产部署](../ch12_production/12_生产部署_zh.md)
