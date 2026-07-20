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
docker compose build lumendb   # Build DeepVector image
docker compose up -d           # Start all services
docker compose logs -f         # View logs
docker compose down            # Stop all services
```

## Related Chapters

- 馃搸 **Ch11** [Docker & Deployment](../ch11_docker/11_Docker涓庨儴缃瞋en.md)
