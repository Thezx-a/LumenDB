# Docker Containerization

## Install Docker

```bash
# Linux
curl -fsSL https://get.docker.com | sh

# Windows: Install Docker Desktop
# https://docs.docker.com/desktop/install/windows-install/
```

## Docker Compose

```bash
# Linux
sudo apt install docker-compose-plugin

# Or standalone
pip install docker-compose
```

## Common Commands

```bash
docker compose build lumendb   # Build LumenDB image
docker compose up -d           # Start all services
docker compose logs -f         # View logs
docker compose down            # Stop all services
```

## Related Chapters

- 📎 **Ch11** [Docker & Deployment](../ch11_docker/11_Docker与部署_en.md)
