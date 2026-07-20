# Docker 容器化 / Docker Containerization

## 安装 Docker / Install Docker

```bash
# Linux
curl -fsSL https://get.docker.com | sh

# Windows: 安装 Docker Desktop
# https://docs.docker.com/desktop/install/windows-install/
```

## Docker Compose

```bash
# Linux
sudo apt install docker-compose-plugin

# 或独立安装 / Or standalone
pip install docker-compose
```

## 常用命令 / Common Commands

```bash
docker compose build deepvector   # 构建 DeepVector 镜像
docker compose up -d           # 启动所有服务
docker compose logs -f         # 查看日志
docker compose down            # 停止所有服务
```

## 相关章节

- 📎 **Ch11** [Docker与部署](../ch11_docker/11_Docker与部署_zh.md)
