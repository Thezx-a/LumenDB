# SkyNet 设计文档
## 1. C++20 协程调度模型
- Task<T>: promise_type 管理生命周期，co_await 链式调用
- Executor: epoll + 就绪队列 + TimerWheel 三路复用
## 2. 网络层
- Socket: asyncRead/asyncWrite 返回 Awaitable，注册到 epoll
- Acceptor: 协程化 accept
- TcpStream: readExact/readUntil 对 HTTP 友好
## 3. HTTP 解析
- 增量状态机: METHOD→PATH→VERSION→HEADER→BODY→DONE
- 支持 chunked + pipelining
## 4. 网关代理
- LoadBalancer: WRR / LeastConn / ConsistentHash
- HealthCheck: 后台线程周期探测
- ConnectionPool: 后端连接复用
