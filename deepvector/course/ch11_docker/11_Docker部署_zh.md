# 绗崄涓€绔狅細Docker 涓庨儴缃?
> 瀹瑰櫒鍖?DeepVector + Agent 鐢熶骇閮ㄧ讲鏂规銆?
## 鍓嶇疆鐭ヨ瘑

> 馃搸 **鍙傝€?*: [Docker瀹瑰櫒鍖朷(../prerequisites/03_Docker瀹瑰櫒鍖朹zh.md) | [鏋勫缓鐜閰嶇疆](../prerequisites/01_鏋勫缓鐜閰嶇疆_zh.md)

---

## 瀛︿範鐩爣

- 鐞嗚В澶氶樁娈?Docker 鏋勫缓
- 鎺屾彙 docker-compose 澶氭湇鍔＄紪鎺?- 瀛︿細鐢熶骇閮ㄧ讲妫€鏌ユ竻鍗?
---

## 11.1 澶氶樁娈垫瀯寤?
```mermaid
flowchart LR
    subgraph Build["鏋勫缓闃舵"]
        G[g++-12 + cmake] --> B["cmake --build"]
        B --> L["lumendb_server 浜岃繘鍒?]
    end
    subgraph Runtime["杩愯闃舵"]
        U["ubuntu:22.04 slim"] --> C["鎷疯礉浜岃繘鍒?]
        C --> S["/usr/local/bin/lumendb_server"]
    end
    Build --> Runtime
```

```dockerfile
# Stage 1: 鏋勫缓 / Build
FROM ubuntu:22.04 AS builder
RUN apt-get install -y g++-12 cmake ninja-build
COPY . .
RUN cmake -B build && cmake --build build --target lumendb_server

# Stage 2: 杩愯 / Runtime
FROM ubuntu:22.04
COPY --from=builder /build/build/lumendb_server /usr/local/bin/
EXPOSE 8080
CMD ["lumendb_server", "--port", "8080"]
```

---

## 11.2 Docker Compose 缂栨帓

```yaml
version: "3.9"
services:
  lumendb:
    build:
      context: .
      target: lumendb-runtime
    ports: ["8080:8080"]
    volumes: ["./data:/data"]
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      
  agent:
    build:
      context: .
      target: agent-runtime
    ports: ["8090:8090"]
    depends_on:
      lumendb:
        condition: service_healthy
    environment:
      - AGENTICDB_LLM_PROVIDER=${LLM_PROVIDER:-ollama}
      - OPENAI_API_KEY=${OPENAI_API_KEY:-}
      
  ollama:
    image: ollama/ollama:latest
    profiles: ["full"]
    volumes: ["ollama_data:/root/.ollama"]
```

---

## 11.3 鐢熶骇妫€鏌ユ竻鍗?
| 绫诲埆 | 妫€鏌ラ」 | 璇存槑 |
|------|--------|------|
| 瀹夊叏 | API Key | DeepVector `--api-key` 闃叉鏈巿鏉?|
| 瀹夊叏 | HTTPS | Let's Encrypt 鍙嶅悜浠ｇ悊 |
| 鍙潬鎬?| 鑷姩閲嶅惎 | systemd/docker restart policy |
| 鍙潬鎬?| 鍋ュ悍妫€鏌?| Docker HEALTHCHECK |
| 鐩戞帶 | 鏃ュ織 | 鏃ュ織杞浆 (logrotate) |
| 鐩戞帶 | 鍛婅 | 鎼滅储寤惰繜 P99 > 500ms |
| 鎬ц兘 | 璧勬簮闄愬埗 | CPU/Memory cgroups |
| 鏁版嵁 | 澶囦唤 | 瀹氭湡澶囦唤 data 鐩綍 |

---

## 鎬濊€冮

1. 澶氶樁娈垫瀯寤轰腑锛屼负浠€涔?builder 闃舵涓嶇洿鎺ヤ娇鐢?`python:3.11` 鑰岀敤 `ubuntu:22.04`锛?2. docker-compose 鐨?`profiles` 鍙傛暟鏈変粈涔堢敤锛?full" profile 閫傚悎浠€涔堝満鏅紵
3. 濡備綍瀹炵幇闆跺仠鏈洪儴缃诧紵涓や釜瀹瑰櫒杞祦鍗囩骇锛?
## 鍔ㄦ墜缁冧範

1. 缁?Docker 闀滃儚娣诲姞鍋ュ悍妫€鏌ワ紝纭繚 lumendb 鍚姩鍚庡啀鍚姩 agent
2. 鍒涘缓涓€涓?`.env.example` 鏂囦欢锛屽垪鍑烘墍鏈夊彲鐢ㄧ殑鐜鍙橀噺
3. 缂栧啓涓€涓?`docker-compose.prod.yml`锛屾坊鍔犺祫婧愰檺鍒跺拰鏃ュ織閰嶇疆
