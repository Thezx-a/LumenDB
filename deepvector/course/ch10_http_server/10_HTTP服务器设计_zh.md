# 第10章：HTTP服务器设计

## 前置知识

> 📎 **参考**: [构建环境配置](../prerequisites/01_构建环境配置.md) — C++ 编译环境与工具链

---

## 目录
1. [HTTP简史](#1-http简史)
2. [什么是Socket？](#2-什么是socket)
3. [文件描述符：通用句柄](#3-文件描述符通用句柄)
4. [TCP Socket分步详解](#4-tcp-socket分步详解)
5. [阻塞与非阻塞I/O](#5-阻塞与非阻塞io)
6. [I/O多路复用：C10K革命](#6-io多路复用c10k革命)
7. [HTTP/1.1协议深入剖析](#7-http11协议深入剖析)
8. [REST API设计](#8-rest-api设计)
9. [认证与授权](#9-认证与授权)
10. [优雅关闭](#10-优雅关闭)
11. [生产环境HTTP服务器](#11-生产环境http服务器)
12. [讨论问题](#12-讨论问题)
13. [练习题](#13-练习题)

---

## 1. HTTP简史

### 1989-1991：万维网的诞生

1989年3月，**Tim Berners-Lee**——一位在**CERN**（Conseil Européen pour la Recherche Nucléaire，欧洲核子研究组织，位于日内瓦）工作的英国科学家——提交了一份题为"信息管理：一份提案"的报告。他的目标是：让物理学家们能够在CERN及之外的计算机网络上共享研究文档。

到1991年底，Berners-Lee已经创建了三项基础技术：

- **HTTP**（超文本传输协议）——一种简单的请求-响应协议，用于获取文档
- **HTML**（超文本标记语言）——一种带有超链接的文档格式语言
- **URL**（统一资源定位符）——一种在网络中定位文档的命名方案

第一个版本**HTTP/0.9**（1991年）按现代标准来看极其简单。它只有一个方法——`GET`——响应是没有头部的原始HTML：

```
GET /index.html\r\n
\r\n
<html>A page of text</html>
```

没有状态码，没有头部，没有版本控制。客户端连接、发送一行数据、接收HTML，然后服务器关闭连接。就这样。协议的完整规范用一段话就能概括。

### 1993-1996：HTTP/1.0与Mosaic的爆发

1993年**Mosaic**——第一个图形化网页浏览器——的发布点燃了万维网。突然间每个人都想上网。HTTP需要成长了。

**HTTP/1.0**（1996年正式化为RFC 1945）增加了：
- **请求和响应头部**——如`Content-Type`、`Content-Length`、`Date`等元数据
- **状态码**——如`200 OK`、`404 Not Found`等数字结果码
- **多种方法**——`GET`、`POST`、`HEAD`
- **版本字符串**——请求行中的`HTTP/1.0`

但HTTP/1.0有一个致命缺陷：**每个请求都需要一个新的TCP连接**。一个加载20张图片的页面需要21个独立的TCP连接。每次连接都需要TCP三次握手（SYN → SYN-ACK → ACK），在数据传输之前大约消耗**1.5个往返时延（RTT）**。在100ms的连接上，每次请求仅连接建立就要浪费150ms。

### 1997：HTTP/1.1与持久连接

**HTTP/1.1**（RFC 2068，后由RFC 2616和RFC 7230更新）通过**持久连接**（默认`Connection: keep-alive`）解决了这个问题。现在一个TCP连接可以承载数十个顺序请求。这个版本在接下来的二十年中主导了万维网。

HTTP/1.1还增加了：
- **Host头部**——每个请求中必须包含，支持虚拟主机（一个IP上多个域名）
- **分块传输编码**——无需预先知道总大小即可流式传输响应
- **内容协商**——客户端和服务器就格式（语言、编码、媒体类型）达成一致
- **缓存控制**——`Cache-Control`、`ETag`、`If-None-Match`头部

### 2000：REST与API革命

2000年，**Roy Fielding**——HTTP/1.1的主要作者之一——发表了博士论文"架构风格与基于网络的软件架构设计"。他在论文中正式描述了**REST**（表述性状态转移），一种他从研究万维网本身中提炼出来的架构风格。

REST成为Web API的主流范式，取代了专有RPC协议（SOAP、XML-RPC、CORBA）的混乱局面。

### 2015-2022：HTTP/2和HTTP/3

**HTTP/2**（RFC 7540，2015年）从文本协议转向了二进制帧，增加了多路复用（一个连接上同时进行多个请求）、头部压缩（HPACK）和服务器推送。

**HTTP/3**（RFC 9114，2022年）完全用**QUIC**——一种由Google开发的协议——取代了TCP。

---

## 2. 什么是Socket？

### 电话类比

**Socket**是一个网络通信端点——一个让两台不同机器上的程序交换数据的抽象。

| 电话类比 | Socket操作 | 系统调用 |
|---------|-----------|---------|
| 买一部电话 | 创建端点 | `socket()` |
| 获得电话号码 | 分配地址 | `bind()` |
| 打开响铃，等待来电 | 标记为被动，排队传入连接 | `listen()` |
| 接听电话 | 接受连接 | `accept()` |
| 与来电者交谈 | 交换数据 | `send()` / `recv()` |
| 挂断电话 | 关闭连接 | `close()` |

### Socket类型

- **SOCK_STREAM**（TCP）——可靠的、有序的、面向连接的字节流
- **SOCK_DGRAM**（UDP）——不可靠的、无序的、无连接的数据报

### 地址族

- **AF_INET**——IPv4地址（32位）
- **AF_INET6**——IPv6地址（128位）
- **AF_UNIX**——Unix域套接字，同一台机器上的进程间通信

---

## 3. 文件描述符：通用句柄

### "一切皆文件"

Unix有一个著名的哲学原则：**"一切皆文件"**。

统一它们的抽象是**文件描述符**（通常缩写为**fd**）：一个表示进程中已打开的I/O资源的小的非负整数。

```
文件描述符表（每进程）：
┌─────┬──────────────────────┐
│  0  │  stdin  (标准输入)     │   ← 由C运行时自动打开
│  1  │  stdout (标准输出)     │
│  2  │  stderr (标准错误)     │
│  3  │  服务器套接字 (端口8080) │   ← socket()返回最低可用的fd
│  4  │  客户端连接1           │
│  5  │  客户端连接2           │
│ ... │  ...                 │
└─────┴──────────────────────┘
```

---

## 4. TCP Socket分步详解

### 浏览器连接时发生了什么

#### 步骤1：服务器设置

1. 创建了套接字：`socket(AF_INET, SOCK_STREAM, 0)`——返回fd 3
2. 设置了`SO_REUSEADDR`——允许重启后立即重新绑定端口
3. 绑定到地址：`bind(fd, "0.0.0.0:8080")`
4. 开始监听：`listen(fd, 128)`

#### 步骤2：TCP三次握手

```
浏览器                            服务器
  │                                │
  │──── SYN (seq=1000) ──────────►│
  │                                │
  │◄─── SYN-ACK (seq=3000, ack=1001) ──│
  │                                │
  │──── ACK (ack=3001) ──────────►│
  │                                │
  │     连接已建立                  │
```

#### 步骤3：HTTP请求

```
GET /api/health HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: Mozilla/5.0\r\n
Accept: */*\r\n
\r\n
```

#### 步骤4：HTTP响应

```
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Content-Length: 15\r\n
Connection: close\r\n
\r\n
{"status":"ok"}
```

#### 步骤5：连接关闭

```mermaid
sequenceDiagram
    participant Browser as 浏览器
    participant Server as 服务器

    Note over Browser,Server: TCP三次握手 (1.5 RTT)
    Browser->>Server: SYN (seq=1000)
    Server-->>Browser: SYN-ACK (seq=3000, ack=1001)
    Browser->>Server: ACK (ack=3001)

    Note over Browser,Server: HTTP请求/响应
    Browser->>Server: GET /api/health HTTP/1.1\r\nHost: ...
    Server-->>Browser: HTTP/1.1 200 OK\r\nContent-Type: ...\r\n{"status":"ok"}

    Note over Browser,Server: TCP四次挥手
    Server-->>Browser: FIN
    Browser->>Server: ACK
    Browser->>Server: FIN
    Server-->>Browser: ACK
```

### SO_REUSEADDR：为什么你需要它

当一个TCP连接关闭时，它会进入**TIME_WAIT**状态，持续时间为**最大段生存期（MSL）**的两倍。`SO_REUSEADDR`表示："我知道我在做什么，即使有TIME_WAIT也让我绑定。"

---

## 5. 阻塞与非阻塞I/O

### 阻塞I/O：默认行为

**阻塞I/O**是最简单的模型。当你调用`recv()`且没有数据可用时，操作系统让你的线程进入睡眠状态。

**阻塞I/O的核心问题**：一个被阻塞在一个连接上的线程无法处理任何其他连接。如果你有10,000个客户端，你需要10,000个线程。

### 非阻塞I/O：不要睡眠，立即告诉我

```cpp
#include <fcntl.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

### 对比：三种I/O模型

| 模型 | 线程行为 | CPU使用 | 可扩展性 | 复杂度 |
|------|---------|---------|---------|--------|
| **阻塞I/O** | 睡眠直到数据到达 | 低（等待时空闲） | 差：每个连接一个线程 | 简单 |
| **非阻塞I/O + 忙轮询** | 在循环中轮询所有fd | 100%（始终在自旋） | 差：每次迭代O(n) | 中等 |
| **事件驱动（多路复用）** | 在`epoll_wait()`中睡眠直到事件触发 | 低（仅在有工作时活跃） | 优秀：一个线程，数千个连接 | 复杂 |

---

## 6. I/O多路复用：C10K革命

### C10K问题

1999年，软件工程师**Dan Kegel**发表了一篇里程碑式的文章"C10K问题"。他问道：**如何构建一个处理10,000个并发连接的服务器？**

### 多路复用的演进

| 系统调用 | 年份 | 最大FD数 | 查找复杂度 | 主要限制 |
|---------|------|---------|-----------|---------|
| **select()** | 1983 (4.2BSD) | 1024 (`FD_SETSIZE`) | O(n)扫描 | 硬编码限制 |
| **poll()** | 1987 (SVR3 Unix) | 无限制 | O(n)扫描 | 仍然是O(n) |
| **epoll()** | 2002 (Linux 2.5) | 无限制 | O(1)仅返回就绪的fd | Linux专属 |
| **kqueue** | 2000 (FreeBSD 4.1) | 无限制 | O(1) | BSD/macOS专属 |
| **io_uring** | 2019 (Linux 5.1) | 无限制 | O(1) | 异步；前沿技术 |

### epoll()：Linux的革命

**epoll**的核心洞察是：**注册一次，永久监控。**

```mermaid
flowchart TD
    A[事件循环] --> B{epoll_wait}
    B -->|事件就绪| C{是server_fd吗?}
    B -->|超时| D[处理定时器]

    C -->|是| E[接受新连接]
    C -->|否| F[recv客户端数据]

    E --> G[add_fd到epoll]
    G --> B

    F --> H{数据读取完毕?}
    H -->|是| I[关闭fd]
    H -->|否| J[handle_data]
    J --> B

    I --> B
    D --> B

    style A fill:#e8f5e9,stroke:#2e7d32
    style B fill:#fff3e0,stroke:#f57c00
    style E fill:#e1f5fe,stroke:#0288d1
    style F fill:#fce4ec,stroke:#c62828
```

```cpp
#include <sys/epoll.h>

class EpollEventLoop {
    int epoll_fd_;
    static constexpr int MAX_EVENTS = 1024;

public:
    EpollEventLoop() {
        epoll_fd_ = epoll_create1(0);
    }

    void add_fd(int fd, uint32_t events) {
        struct epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    }

    void run(int server_fd) {
        add_fd(server_fd, EPOLLIN);
        struct epoll_event events[MAX_EVENTS];

        while (true) {
            int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;

                if (fd == server_fd) {
                    while (true) {
                        int client = accept4(server_fd, nullptr, nullptr,
                                            SOCK_NONBLOCK);
                        if (client < 0) break;
                        add_fd(client, EPOLLIN | EPOLLET);
                    }
                } else {
                    char buf[4096];
                    while (true) {
                        ssize_t n = recv(fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            handle_data(fd, buf, n);
                        } else if (n == 0) {
                            close(fd);
                            break;
                        } else if (errno == EAGAIN) {
                            break;
                        } else {
                            close(fd);
                            break;
                        }
                    }
                }
            }
        }
    }
};
```

### LT与ET：两种触发模式

| 模式 | 名称 | 行为 | 何时使用 |
|------|------|------|---------|
| **LT** | 水平触发（默认） | 只要缓冲区有数据就通知你 | 简单安全 |
| **ET** | 边缘触发 | 只在状态改变时通知你一次 | 高性能，更复杂 |

---

## 7. HTTP/1.1协议深入剖析

### HTTP请求结构

```
POST /api/v1/search HTTP/1.1\r\n           ← 请求行：方法  路径  版本
Host: localhost:8080\r\n                    ← 头部（键值对）
Content-Type: application/json\r\n
Content-Length: 45\r\n
Authorization: Bearer token123\r\n
\r\n                                        ← 空行 = 头部结束
{"query": [0.1, 0.2, 0.3], "top_k": 10}    ← 主体
```

### HTTP响应结构

```
HTTP/1.1 200 OK\r\n                         ← 状态行：版本  状态码  原因短语
Content-Type: application/json\r\n          ← 头部
Content-Length: 62\r\n
Connection: keep-alive\r\n
\r\n                                        ← 空行 = 头部结束
{"results": [{"id": 42, "score": 0.95}]}   ← 主体
```

### 状态码：结果的语言

```
1xx 信息性
  100 Continue          服务器已接收请求头
  101 Switching Protocols  服务器同意升级

2xx 成功
  200 OK                请求成功
  201 Created           新资源已创建
  204 No Content        成功，但没有主体返回

3xx 重定向
  301 Moved Permanently  资源永久移动到新URL
  304 Not Modified      缓存响应仍然有效

4xx 客户端错误
  400 Bad Request       格式错误的请求
  401 Unauthorized      未提供认证凭据
  403 Forbidden         已认证但未授权访问此资源
  404 Not Found         此URI处不存在资源
  405 Method Not Allowed  不支持的HTTP方法
  422 Unprocessable Entity  语法有效但语义错误
  429 Too Many Requests   超出速率限制

5xx 服务器错误
  500 Internal Server Error  服务器上出了问题
  502 Bad Gateway           上游服务器返回了无效响应
  503 Service Unavailable   服务器暂时过载或停机维护
```

### 头部：元数据层

| 头部 | 用途 | 示例 |
|------|------|------|
| `Host` | 此请求针对哪个域名 | `Host: example.com` |
| `Content-Type` | 主体的MIME类型 | `Content-Type: application/json` |
| `Content-Length` | 主体的字节长度 | `Content-Length: 45` |
| `Authorization` | 认证凭据 | `Authorization: Bearer sk-...` |
| `Connection` | 连接管理 | `Connection: keep-alive` |

### JSON：通用交换格式

```json
{
    "id": 42,
    "vector": [0.1, 0.2, 0.3],
    "metadata": {
        "source": "document.pdf",
        "page": 5
    }
}
```

---

## 8. REST API设计

### 什么是REST？

**REST**（表述性状态转移）是**Roy Fielding**在其2000年博士论文中描述的架构风格。REST的核心约束：

1. **资源通过URI标识**
2. **统一接口**：操作使用HTTP方法
3. **无状态**：每个请求包含服务器需要的所有信息
4. **表述**：资源以表述（JSON、XML等）的形式传输
5. **分层系统**：中间件可以位于客户端和服务器之间

### DeepVector的RESTful路由

```
POST   /api/v1/vectors         插入一个向量
GET    /api/v1/vectors/{id}    检索一个向量
DELETE /api/v1/vectors/{id}    删除一个向量
POST   /api/v1/search          相似性搜索
GET    /api/v1/health          健康检查
GET    /api/v1/stats           统计信息
```

### 统一JSON响应格式

```json
// 成功
{
    "status": "ok",
    "data": { "id": 42, "results": [{"id": 7, "score": 0.95}] }
}

// 错误
{
    "status": "error",
    "error": {
        "code": "DIMENSION_MISMATCH",
        "message": "Expected dimension 768, got 1024"
    }
}
```

### 路由器：交通警察

```cpp
class Router {
    std::unordered_map<std::string,
        std::function<HttpResponse(HttpRequest&)>> routes_;

public:
    void get(const std::string& path,
             std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["GET " + path] = handler;
    }
    void post(const std::string& path,
              std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["POST " + path] = handler;
    }

    HttpResponse dispatch(HttpRequest& req) {
        std::string key = req.method + " " + req.path;
        auto it = routes_.find(key);
        if (it != routes_.end()) {
            return it->second(req);
        }
        return HttpResponse(404,
            R"({"status":"error","error":{"code":"NOT_FOUND"}})");
    }
};
```

### CORS：跨源资源共享

**CORS**（跨源资源共享）是一种控制哪些网页可以从不同域名请求资源的安全机制。

---

## 9. 认证与授权

### Bearer Token认证

```
Authorization: Bearer sk-lumen-test-token
```

### JWT：自包含令牌

**JWT**（JSON Web Token）是RFC 7519中定义的自包含、可验证的令牌格式。

```
eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1c2VyMTIzIiwiZXhwIjoxNjkwMDg2NDAwfQ.abc123def456
├── 头部 ──────────┤├── 载荷 ───────────────────────────┤├── 签名 ───┤
```

- **头部**：签名算法
- **载荷**：声明——用户ID、过期时间、权限
- **签名**：使用密钥的HMAC或RSA签名

### 刷新令牌+访问令牌模式

- **访问令牌**：短寿命（5-15分钟），用于API调用
- **刷新令牌**：长寿命（数天/数周），仅用于获取新的访问令牌

---

## 10. 优雅关闭

### 优雅关闭：五个步骤

```
1. 接收SIGTERM ──► 设置标志：g_running = false
2. close(listen_fd) ──► 停止接受新连接
3. 排空进行中的请求 ──► 让当前请求完成（宽限期，通常10-30秒）
4. 持久化状态 ──► 将索引刷新到磁盘，写入WAL
5. exit(0) ──► 干净退出
```

### 完整实现

```cpp
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signal_handler(int sig) {
    g_running = 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int server_fd = create_server(8080);

    while (g_running) {
        epoll_wait(...);
    }

    printf("Shutting down gracefully...\n");
    close(server_fd);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!active_connections.empty() &&
           std::chrono::steady_clock::now() < deadline) {
        poll_remaining_connections(100);
    }

    for (int fd : active_connections) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    index_.flush();
    printf("Shutdown complete.\n");
    return 0;
}
```

---

## 11. 生产环境HTTP服务器

### nginx

**nginx** 为约 33-34% 的网站提供服务（W3Techs, 2025）。它使用事件驱动（epoll/kqueue）模型处理所有I/O。

### 为什么事件驱动模型占据主导地位

1. **无线程开销**：一个线程处理数千个连接
2. **无锁**：单线程 = 没有互斥锁、没有竞态条件、没有死锁
3. **可预测的延迟**：没有上下文切换、没有锁争用
4. **高效的内存使用**：一个栈代替数千个栈

代价：代码更复杂。你不能在事件循环中进行阻塞调用。

---

## 12. 讨论问题

1. 为什么`select()`的`FD_SETSIZE`被限制为1024？这是技术限制还是设计选择？
2. 在epoll的ET模式下，如果你在到达`EAGAIN`之前跳出`recv`循环，剩余数据什么时候会被处理？
3. HTTP/1.1的`Connection: keep-alive`和HTTP/2的多路复用之间的根本区别是什么？
4. 如果客户端发送的`Content-Length`与实际主体长度不匹配，服务器应如何防御？
5. 设计一个滑动窗口速率限制器
6. 无状态JWT认证如何处理"用户被封禁但令牌尚未过期"？
7. `SO_REUSEADDR`与`SO_REUSEPORT`分别解决什么问题？
8. 在`close(fd)`之后，fd编号立即被`accept()`重用。新连接是否会意外读取旧连接的过期数据？

---

## 13. 练习题

### 练习1：单线程epoll HTTP服务器（35分钟）

使用epoll（ET模式）从零构建一个HTTP服务器，支持：
- `GET /health` → `200 OK {"status":"ok"}`
- `POST /echo` → 返回请求主体
- 手动HTTP请求解析

### 练习2：完整REST API（30分钟）

为向量数据库实现CRUD REST API。

### 练习3：认证中间件（20分钟）

实现Bearer token认证中间件。

### 练习4：优雅关闭+Keep-Alive（25分钟）

### 练习5：压力测试（20分钟）

```bash
wrk -t 4 -c 100 -d 30s --latency http://localhost:8080/api/v1/health
```

---

## 章节总结

| 层 | 核心技术 | 关键概念 |
|----|---------|---------|
| **传输层** | Berkeley Sockets API | fd, socket() → bind() → listen() → accept() → send()/recv() → close() |
| **多路复用** | select → poll → epoll/kqueue | C10K问题；LT与ET；O(1)事件通知 |
| **协议层** | HTTP/1.1 | 请求行 + 头部 + 主体；Keep-Alive；Content-Length与chunked |
| **应用层** | REST API | 资源URI + HTTP方法（CRUD）；统一JSON响应；状态码 |
| **安全层** | 认证 | Bearer Token；JWT三部分结构；认证与授权 |
| **运维层** | 信号处理 | SIGTERM → 排空请求 → 持久化 → 退出；宽限期 |

> 下一章：[第11章：C++20协程与SkyNet](../ch11_coroutines/README.md)