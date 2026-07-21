# Module 04 — Go 与 TypeScript 基础

> 对应规划：[go.mod](file:///c:/Users/Administrator/Desktop/hellocpp/go.mod)（`github.com/titan-kv/titan`）、REFACTORING.md Phase 3-6（gateway/services/web）

## 1. 核心知识

- Go 语言定位：为后端微服务而生，原生并发（goroutine + channel）、静态编译、跨平台单文件部署。
- Go 并发三件套：`goroutine`（`go f()`）、`channel`（`chan T`）、`select`；CSP 通信顺序进程模型。
- Go 接口：隐式实现（鸭子类型），`error` 是普通接口，惯用 `(T, error)` 多返回值。
- gRPC + Protobuf：跨语言 RPC，强类型 IDL，双向流；TitanKV 用其打通 Go 网关 ↔ C++ 存储引擎。
- TypeScript：JavaScript 的超集 + 静态类型，`type` / `interface` / 泛型。
- Next.js App Router：React 服务端组件（RSC）、文件即路由、Server Actions、TanStack Query 数据获取。

## 2. 内容详解

### 2.1 为什么 TitanKV 用 Go 做业务层

架构分工（来自 [README.md](file:///c:/Users/Administrator/Desktop/hellocpp/README.md)）：

- **C++ 存储引擎**（minikv）：追求极致性能，零开销、内存可控。
- **Go 微服务**（gateway/services）：追求开发效率与并发表达力，gRPC 串联。

Go 的优势在此场景：

- goroutine 极轻量（~2KB 栈 vs C++ 线程 ~1MB），百万并发无压力。
- `net/http` + `google.golang.org/grpc` 生态成熟，写网关/认证/限流中间件快。
- 静态编译单二进制，容器镜像小、部署简单。
- 与 C++ 通过 cgo 或 gRPC 互通（Phase 2 计划）。

### 2.2 goroutine 与 channel

```go
// 生产者-消费者（对比 Module 03 的 C++ ThreadPool）
func producer(ch chan<- int) {
    for i := 0; i < 100; i++ { ch <- i }
    close(ch)
}
func consumer(ch <-chan int, done chan<- struct{}) {
    for v := range ch { fmt.Println(v) }
    done <- struct{}{}
}
func main() {
    ch := make(chan int, 10)            // 带缓冲 channel
    done := make(chan struct{})
    go producer(ch); go consumer(ch, done)
    <-done                              // 阻塞等待完成
}
```

要点：

- `go f()` 启动 goroutine，由 Go runtime 调度（M:N 用户态调度，抢占式）。
- `chan T` 是并发安全的 FIFO 队列；带缓冲 channel 类似有界阻塞队列。
- `range ch` 持续接收直到 channel 被 `close`。
- `select { case ...: }` 多路复用 channel，类似 epoll 但用于 channel。
- 对比 C++：C++ 需要 `mutex + condition_variable` 手搓，Go 用 channel 一行搞定，但 channel 有运行时开销。

### 2.3 接口与错误处理

```go
type Storage interface {
    Get(ctx context.Context, key string) (string, error)
    Put(ctx context.Context, key, val string) error
}

type Engine struct{ db *C.DBImpl }   // cgo 包装或 gRPC client
func (e *Engine) Get(ctx context.Context, key string) (string, error) {
    val, err := e.db.Get(key)
    if err != nil { return "", fmt.Errorf("get %q: %w", key, err) }
    return val, nil
}
```

- 接口**隐式实现**：`Engine` 实现了 `Get`/`Put` 方法即满足 `Storage`，无需 `implements` 声明。
- 错误是值：`(T, error)` 多返回值；`fmt.Errorf("%w", err)` 包裹错误形成错误链，`errors.Is`/`errors.As` 解包。
- `context.Context` 贯穿调用链，支持超时、取消、传值——微服务必备。

### 2.4 gRPC + Protobuf

TitanKV 计划在 `proto/keyforge/storage.proto` 定义 Put/Get/Delete/Scan 接口（见 REFACTORING.md Phase 2）。示例：

```protobuf
service Storage {
  rpc Put(PutRequest) returns (PutResponse);
  rpc Get(GetRequest) returns (GetResponse);
  rpc Scan(ScanRequest) returns (stream ScanEntry);   // 服务端流
}
message PutRequest { bytes key = 1; bytes value = 2; }
```

- Protobuf 是二进制 IDL，比 JSON 更小更快，强类型。
- gRPC 基于 HTTP/2 多路复用，支持一元/服务端流/客户端流/双向流四种调用。
- `protoc` 生成 Go 与 C++ 桩代码，两端用同一份 proto 保证协议一致。

### 2.5 TypeScript 类型系统

```typescript
type Collection = {
  id: string;
  name: string;
  createdAt: Date;
  settings: { shards: number; replication: number };
};

interface CollectionRepo {
  list(): Promise<Collection[]>;
  create(input: Omit<Collection, 'id' | 'createdAt'>): Promise<Collection>;
}
```

- `type` vs `interface`：`interface` 可声明合并，`type` 支持联合/交叉/条件类型；日常二者皆可。
- 工具类型：`Omit`/`Pick`/`Partial`/`Record` 减少重复定义。
- 类型只在编译期存在，编译后擦除（与 C++ 模板类似但更轻）。

### 2.6 Next.js App Router

TitanKV 控制台（Phase 6）规划用 Next.js 15 App Router + Tailwind + shadcn/ui + TanStack Query。目录即路由：

```
web/app/
├── layout.tsx           根布局
├── page.tsx             首页（仪表盘）
├── data/page.tsx        /data 数据浏览器
├── collections/page.tsx /collections
└── api/                 路由处理器（BFF）
```

- **服务端组件（RSC）**：默认在服务端渲染，可直连数据库，无 JS 发到客户端。
- **客户端组件**：`'use client'` 指令，用于交互（useState、事件）。
- **TanStack Query**：客户端数据缓存、失效、乐观更新；与服务端组件互补。

## 3. 思考题

1. goroutine 和 C++ 的 `std::thread`、`std::coroutine` 各有什么本质区别？
2. Go 用 `channel` 传递所有权，C++ 用 `std::move` 转移所有权，二者哲学上有何异同？
3. Go 的接口是隐式实现，相比 C++ 的显式虚函数继承，优势和劣势各是什么？
4. gRPC 相比 REST+JSON 在 TitanKV 这种内部服务间通信中有什么优势？什么场景下反而该用 REST？
5. Next.js 服务端组件默认不发送 JS 到客户端，这对仪表盘这种实时数据页面意味着什么？何时必须切到客户端组件？

## 4. 动手题

### 题 4.1（Go 并发限流器）

用 `time.Ticker` + `chan struct{}` 实现一个令牌桶限流器：`Allow() bool`，每秒发放 N 个令牌。用 1000 个 goroutine 并发请求验证限流效果。

### 题 4.2（gRPC 最小示例）

定义 `proto/storage.proto`（Put/Get），用 `protoc-gen-go` + `protoc-gen-go-grpc` 生成桩代码，写一个 server 和 client，实现内存 map 后端。运行通一次 Put/Get。

### 题 4.3（Next.js 仪表盘骨架）

用 `npx create-next-app@latest` 创建项目（App Router + TypeScript + Tailwind），做一个 `/dashboard` 页面：用服务端组件 fetch 假数据，渲染 QPS/延迟卡片；点击「刷新」用客户端组件 + TanStack Query 重新拉取。

## 5. 自检

1. Go 启动 goroutine 的关键字是____，调度模型是____（M:N / 1:1）。
2. 带缓冲 channel 满 时发送方____，空 时接收方____。
3. Go 接口实现是____（显式/隐式），错误以____形式返回。
4. gRPC 基于____协议，支持____种调用模式。
5. Next.js App Router 中____（服务端/客户端）组件默认不发送 JS 到浏览器。

<details>
<summary>参考答案</summary>

1. `go`；M:N
2. 阻塞；阻塞
3. 隐式；值（`(T, error)`）
4. HTTP/2；四（一元、服务端流、客户端流、双向流）
5. 服务端

思考题要点：
1. `std::thread` 是 1:1 内核线程，开销大；`std::coroutine` 是协作式无栈协程，需手写调度器；goroutine 是 M:N 抢占式有栈协程，runtime 调度，自带运行时。
2. channel 是运行时同步原语（带锁/调度），move 是编译期语义转换；channel 隐式传递所有权且阻塞同步，move 零开销但不同步。哲学上：Go 鼓励「通过通信共享内存」，C++ 鼓励「通过共享内存通信」。
3. 优势：解耦、易 mock、组合灵活；劣势：隐式导致实现关系不直观、重构难追踪、接口满意度需工具辅助检查。
4. 优势：二进制更小、强类型、HTTP/2 多路复用低延迟、流式；REST 适合对外的、需浏览器/curl 直接访问的、需缓存（HTTP 语义）的场景。
5. 意味着首屏快、SEO 好、bundle 小；但实时数据需轮询/SSE/WebSocket，必须切到客户端组件（`'use client'`）使用 hooks。

</details>

---

← [Module 03](./03-modern-cpp.md)  |  下一模块：[Module 05 — 跳表与有序结构](./05-skiplist.md) →
