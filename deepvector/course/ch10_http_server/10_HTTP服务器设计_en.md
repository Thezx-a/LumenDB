# Chapter 10: HTTP Server Design

## Prerequisites

This chapter assumes familiarity with the following concepts. Review these shared documents before proceeding:

> 📎 **Reference**: [Build Environment Configuration](../prerequisites/01_构建环境配置_en.md) — Build tools and CMake setup

---

## Table of Contents
1. [A Brief History of HTTP](#1-a-brief-history-of-http)
2. [What is a Socket?](#2-what-is-a-socket)
3. [File Descriptors: The Universal Handle](#3-file-descriptors-the-universal-handle)
4. [TCP Sockets Step by Step](#4-tcp-sockets-step-by-step)
5. [Blocking vs Non-Blocking I/O](#5-blocking-vs-non-blocking-io)
6. [I/O Multiplexing: The C10K Revolution](#6-io-multiplexing-the-c10k-revolution)
7. [HTTP/1.1 Protocol Deep Dive](#7-http11-protocol-deep-dive)
8. [REST API Design](#8-rest-api-design)
9. [Authentication and Authorization](#9-authentication-and-authorization)
10. [Graceful Shutdown](#10-graceful-shutdown)
11. [Production HTTP Servers](#11-production-http-servers)
12. [Discussion Questions](#12-discussion-questions)
13. [Exercises](#13-exercises)

---

## 1. A Brief History of HTTP

### 1989-1991: The Birth of the Web

In March 1989, **Tim Berners-Lee**, a British scientist working at **CERN** (Conseil Européen pour la Recherche Nucléaire, the European Organization for Nuclear Research in Geneva), submitted a proposal titled "Information Management: A Proposal." His goal: let physicists share research documents across a network of computers at CERN and beyond.

By the end of 1991, Berners-Lee had created three foundational technologies:

- **HTTP** (HyperText Transfer Protocol) — a simple request-response protocol for fetching documents
- **HTML** (HyperText Markup Language) — a formatting language for documents with hyperlinks
- **URL** (Uniform Resource Locator) — a naming scheme for addressing documents on the network

The first version, **HTTP/0.9** (1991), was absurdly simple by modern standards. It had exactly one method — `GET` — and responses were raw HTML with no headers:

```
GET /index.html\r\n
\r\n
<html>A page of text</html>
```

There were no status codes, no headers, no versioning. A client connected, sent a line, received HTML, and the server closed the connection. That was it. The protocol fit in a single paragraph of specification.

### 1993-1996: HTTP/1.0 and the Mosaic Explosion

The release of **Mosaic** (1993), the first graphical web browser, ignited the web. Suddenly everyone wanted to be on the internet. HTTP needed to grow up.

**HTTP/1.0** (formalized as RFC 1945 in 1996) added:
- **Request and response headers** — metadata like `Content-Type`, `Content-Length`, `Date`
- **Status codes** — numeric results like `200 OK`, `404 Not Found`
- **Multiple methods** — `GET`, `POST`, `HEAD`
- **A version string** — `HTTP/1.0` in the request line

But HTTP/1.0 had a critical flaw: **every request required a new TCP connection**. A browser loading a page with 20 images needed 21 separate TCP connections. Each connection required a TCP three-way handshake (SYN → SYN-ACK → ACK), consuming roughly **1.5 round-trip times (RTT)** before any data could flow. On a 100ms connection, that's 150ms wasted per request just for connection setup.

### 1997: HTTP/1.1 and Keep-Alive

**HTTP/1.1** (RFC 2068, later updated by RFC 2616 and RFC 7230) fixed this with **persistent connections** (`Connection: keep-alive` by default). Now a single TCP connection could carry dozens of sequential requests. This was the version that would dominate the web for two decades.

HTTP/1.1 also added:
- **Host header** — required in every request, enabling virtual hosting (multiple domains on one IP)
- **Chunked transfer encoding** — streaming responses without knowing the total size upfront
- **Content negotiation** — client and server agree on format (language, encoding, media type)
- **Cache control** — `Cache-Control`, `ETag`, `If-None-Match` headers

### 2000: REST and the API Revolution

In 2000, **Roy Fielding** — one of the principal authors of HTTP/1.1 — published his doctoral dissertation, "Architectural Styles and the Design of Network-based Software Architectures." In it, he formally described **REST** (REpresentational State Transfer), an architectural style he'd derived from studying the web itself.

REST wasn't invented from scratch. Fielding was *describing* what already worked about the web: resources identified by URIs, stateless request-response interactions, a uniform interface of HTTP methods. He gave a name to the implicit architecture that had evolved organically at CERN and beyond.

REST became the dominant paradigm for web APIs, replacing the chaos of proprietary RPC protocols (SOAP, XML-RPC, CORBA). By 2010, RESTful APIs had won completely.

### 2015-2022: HTTP/2 and HTTP/3

**HTTP/2** (RFC 7540, 2015) moved from a text-based protocol to binary frames, added multiplexing (multiple requests in flight simultaneously on one connection), header compression (HPACK), and server push. It still runs over TCP.

**HTTP/3** (RFC 9114, 2022) replaced TCP entirely with **QUIC** (Quick UDP Internet Connections), a protocol developed by Google. QUIC runs over **UDP** (User Datagram Protocol), the simpler, connectionless sibling of TCP. By avoiding TCP's head-of-line blocking (where one slow stream blocks all others), HTTP/3 delivers better performance on lossy networks.

### Why This Matters for Your HTTP Server

You'll implement HTTP/1.1 — the simplest version that's still universally supported. Understanding the history gives you context for every design decision: why headers exist, why keep-alive matters, why status codes are three-digit numbers, and why the protocol looks the way it does.

---

## 2. What is a Socket?

### The Telephone Analogy

A **socket** is a network communication endpoint — an abstraction that lets two programs on different machines exchange data. The best analogy is a telephone:

| Telephone Analogy | Socket Operation | System Call |
|-------------------|------------------|-------------|
| Buy a phone | Create an endpoint | `socket()` |
| Get a phone number | Assign an address | `bind()` |
| Turn on ringer, wait for calls | Mark as passive, queue incoming connections | `listen()` |
| Pick up the phone | Accept a connection | `accept()` |
| Talk to the caller | Exchange data | `send()` / `recv()` |
| Hang up | Close the connection | `close()` |

This interface was designed by **Bill Joy** at UC Berkeley in 1983 for the **4.2BSD Unix** operating system, and is known as the **Berkeley Sockets API**. It is so fundamental that every operating system — Linux, Windows (as Winsock), macOS, FreeBSD, even mobile platforms — implements nearly the same API. Four decades later, `socket()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`, `close()` remain the universal vocabulary of network programming.

### Socket Types

There are two main socket types you'll encounter:

- **SOCK_STREAM** (TCP) — Reliable, ordered, connection-oriented byte stream. Like a phone call: you establish a connection, then exchange a stream of bytes. The kernel guarantees delivery and ordering. Used for HTTP, SSH, SMTP, and most application protocols.

- **SOCK_DGRAM** (UDP) — Unreliable, unordered, connectionless datagrams. Like postal mail: you send a letter with an address, and it may arrive, may arrive out of order, or may not arrive at all. Used for DNS lookups, video streaming, VoIP, and games where speed matters more than reliability.

### Address Families

- **AF_INET** — IPv4 addresses (32-bit, e.g., `192.168.1.1`). The classic internet address format.
- **AF_INET6** — IPv6 addresses (128-bit, e.g., `2001:0db8::1`). The successor to IPv4, with a vastly larger address space.
- **AF_UNIX** — Unix domain sockets. Communication between processes on the *same machine* using filesystem paths (e.g., `/var/run/docker.sock`). Faster than TCP because there's no network stack overhead.

---

## 3. File Descriptors: The Universal Handle

### "Everything is a File"

Unix has a famous philosophical principle: **"Everything is a file"**. A disk file, a pipe (inter-process communication channel), a network socket, a terminal, even `/dev/random` (a random number generator) — all are accessed through the same set of system calls: `open()`, `read()`, `write()`, `close()`.

The abstraction that unifies them all is the **file descriptor** (often abbreviated **fd**): a small non-negative integer that represents an open I/O resource within a process.

### The File Descriptor Table

Every process has a **file descriptor table** — a per-process array managed by the kernel:

```
File Descriptor Table (per process):
┌─────┬──────────────────────┐
│  0  │  stdin  (standard input)     │   ← automatically opened by the C runtime
│  1  │  stdout (standard output)    │
│  2  │  stderr (standard error)     │
│  3  │  server socket (port 8080)   │   ← socket() returns the lowest available fd
│  4  │  client connection 1         │
│  5  │  client connection 2         │
│ ... │  ...                         │
└─────┴──────────────────────┘
```

Key properties:
- **File descriptors are integers**, not pointers. They're indices into an in-kernel table.
- **They start at 0** (stdin), 1 (stdout), 2 (stderr). The next `open()` or `socket()` returns 3.
- **They are process-local.** Two processes can have fd 5 pointing to completely different resources.
- **They are limited.** The Linux kernel default limit is `ulimit -n` (typically 1024, configurable to millions). This is the **C10K problem** in microcosm.

### Why This Matters

When you call `socket()`, you get back an `int`. When you call `accept()`, you get back another `int`. When you call `recv(fd, ...)` or `send(fd, ...)`, you pass that `int`. Everything in network programming revolves around managing these integers. Understanding file descriptors is the foundation of understanding how event loops work — because `select()`, `poll()`, and `epoll()` all take arrays of file descriptors as input.

---

## 4. TCP Sockets Step by Step

### What Happens When a Browser Connects

Let's trace the complete lifecycle of a TCP connection from the moment a user types `http://localhost:8080/api/health` in their browser to the moment the response arrives.

#### Step 1: Server Setup

Before any client connects, the server has already:

1. Created a socket: `socket(AF_INET, SOCK_STREAM, 0)` — returns fd 3
2. Set `SO_REUSEADDR` — allows rebinding to a port immediately after restart
3. Bound to an address: `bind(fd, "0.0.0.0:8080")` — "listen on all network interfaces, port 8080"
4. Started listening: `listen(fd, 128)` — "queue up to 128 pending connections, the rest get rejected"

The server now sits in a loop, calling `accept()` which blocks (sleeps) until a client connects.

#### Step 2: TCP Three-Way Handshake

When the browser's TCP stack initiates a connection, the **three-way handshake** occurs:

```
Browser                          Server
  │                                │
  │──── SYN (seq=1000) ──────────►│  "I want to connect"
  │                                │
  │◄─── SYN-ACK (seq=3000, ack=1001) ──│  "OK, I acknowledge your seq, here's mine"
  │                                │
  │──── ACK (ack=3001) ──────────►│  "I acknowledge your seq"
  │                                │
  │     Connection ESTABLISHED     │
```

This handshake consumes **1.5 RTT** (one and a half round-trip times). On a 50ms connection, that's 75ms before any HTTP data flows. This is why keep-alive matters — you only pay this cost once per connection.

After the handshake, the kernel moves the connection from the listen queue to an "established" queue. `accept()` wakes up, removes the connection from the queue, and returns a **new file descriptor** representing this specific client connection.

#### Step 3: HTTP Request

The browser sends an HTTP request as bytes over the TCP connection:

```
GET /api/health HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: Mozilla/5.0\r\n
Accept: */*\r\n
\r\n
```

The TCP stack on the server reassembles these bytes (TCP may split them across multiple segments) and delivers them in order to the `recv()` call.

#### Step 4: Server Processing

The server reads the bytes with `recv(client_fd, buffer, ...)`, parses the HTTP request (method, path, headers, body), routes it to the appropriate handler, and generates a response.

#### Step 5: HTTP Response

The server writes the response with `send(client_fd, response, ...)`:

```
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Content-Length: 15\r\n
Connection: close\r\n
\r\n
{"status":"ok"}
```

#### Step 6: Connection Close

With `Connection: close`, the server initiates a **TCP four-way teardown**:

```
Browser                          Server
  │                                │
  │◄─── FIN ──────────────────────│  "I'm done sending"
  │──── ACK ─────────────────────►│  "Acknowledged"
  │                                │
  │──── FIN ──────────────────────►│  "I'm done too"
  │◄─── ACK ──────────────────────│  "Acknowledged"
  │                                │
  │     Connection CLOSED          │
```

The server closes the file descriptor, and the kernel reclaims the connection resources.

```mermaid
sequenceDiagram
    participant Browser
    participant Server

    Note over Browser,Server: TCP Three-Way Handshake (1.5 RTT)
    Browser->>Server: SYN (seq=1000)
    Server-->>Browser: SYN-ACK (seq=3000, ack=1001)
    Browser->>Server: ACK (ack=3001)

    Note over Browser,Server: HTTP Request/Response
    Browser->>Server: GET /api/health HTTP/1.1\r\nHost: ...
    Server-->>Browser: HTTP/1.1 200 OK\r\nContent-Type: ...\r\n{"status":"ok"}

    Note over Browser,Server: TCP Four-Way Teardown
    Server-->>Browser: FIN
    Browser->>Server: ACK
    Browser->>Server: FIN
    Server-->>Browser: ACK
```

### SO_REUSEADDR: Why You Need It

When a TCP connection closes, it enters a **TIME_WAIT** state for twice the **Maximum Segment Lifetime (MSL)** — typically 60 seconds on Linux. During TIME_WAIT, the kernel refuses to let another socket bind to the same port.

Why? Because old packets from the previous connection might still be in transit. If you immediately started a new server on the same port, those stale packets could be misinterpreted as belonging to the new connection, causing data corruption. TIME_WAIT prevents this.

`SO_REUSEADDR` says: "I know what I'm doing, let me bind even if there's a TIME_WAIT." Without it, restarting your server after a crash takes 1-2 minutes.

---

## 5. Blocking vs Non-Blocking I/O

### Blocking I/O: The Default

**Blocking I/O** is the simplest model. When you call `recv()` and no data is available, the operating system puts your thread to sleep. It wakes up only when data arrives or the connection is closed. During this time, your thread cannot do anything else.

```cpp
// Blocking recv: if the client doesn't send data, this line never returns
char buf[4096];
ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
//    ▲
//    └── Thread sleeps here, waiting for the kernel to say "data arrived"
printf("received %zd bytes\n", n);
```

**The core problem with blocking I/O**: a thread blocked on one connection cannot handle any other connection. If you have 10,000 clients, you need 10,000 threads. Each thread costs ~1MB of stack memory (10GB total), and the kernel must context-switch between them — which is expensive.

### Non-Blocking I/O: Don't Sleep, Tell Me Immediately

**Non-blocking I/O** tells the OS: "If data isn't ready, don't put me to sleep — return an error immediately so I can do other work."

```cpp
#include <fcntl.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Non-blocking recv
char buf[4096];
ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // "Data hasn't arrived yet, try again later"
        return;
    }
    // Real error (connection closed, etc.)
    perror("recv");
    close(client_fd);
}
```

`EAGAIN` and `EWOULDBLOCK` are the two names for the same error code on Linux (historical coincidence). They mean: "The operation would have blocked, but you asked for non-blocking mode, so here's an error instead."

### The Busy-Wait Trap

The natural impulse with non-blocking I/O is to poll in a tight loop:

```cpp
// BAD: Busy polling — burns 100% CPU doing nothing useful
while (true) {
    for (int fd : all_clients) {
        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0) handle_data(fd, buf, n);
    }
}
```

This is called **busy polling** or **spinning**. The CPU cycles through all file descriptors checking if any have data ready. Most of the time, none of them do — so you're burning CPU for nothing.

The solution: let the **kernel** tell you which file descriptors are ready. That's I/O multiplexing.

### Comparison: Three I/O Models

```mermaid
pie title I/O Model Use Case Comparison
    "Blocking I/O - Low-Concurrency Simple Scenarios" : 15
    "Non-blocking I/O + Busy Polling - Not Recommended" : 5
    "Event-Driven I/O (epoll) - High-Concurrency Production" : 60
    "Thread Pool + Blocking I/O - Medium Concurrency" : 20
```

| Model | Thread Behavior | CPU Usage | Scalability | Complexity |
|-------|-----------------|-----------|-------------|------------|
| **Blocking I/O** | Sleeps until data arrives | Low (idle while waiting) | Poor: one thread per connection | Simple |
| **Non-blocking I/O + busy poll** | Polls all fds in a loop | 100% (always spinning) | Poor: O(n) per iteration | Moderate |
| **Event-driven (multiplexed)** | Sleeps in `epoll_wait()` until an event fires | Low (only active when work exists) | Excellent: one thread, thousands of connections | Complex |

The event-driven model is what nginx, Node.js, and Redis use. It's the industry standard for high-concurrency servers.

---

## 6. I/O Multiplexing: The C10K Revolution

### The C10K Problem

In 1999, software engineer **Dan Kegel** published a landmark essay titled "The C10K Problem." He asked: **how do you build a server that handles 10,000 simultaneous connections?**

At the time, the dominant model was "one thread per connection" — Apache HTTP Server's default. Ten thousand threads needed ~80GB of stack space and caused enormous context-switch overhead. Kegel's analysis showed that the thread-per-connection model couldn't scale, and the industry needed event-driven architectures.

The C10K problem drove the development of scalable I/O multiplexing mechanisms: `epoll` on Linux, `kqueue` on FreeBSD/macOS, and IOCP on Windows.

### What is I/O Multiplexing?

**I/O multiplexing** means monitoring multiple file descriptors simultaneously and being notified only when one or more of them are ready for I/O. Instead of checking each fd one by one (O(n) polling), you register all your fds with the kernel once, and the kernel wakes you up only when something happens.

Think of it like a restaurant host managing tables:
- **Blocking I/O**: One waiter per table, standing there watching each table until someone needs something. 100 tables need 100 waiters.
- **Busy polling**: One waiter running between all 100 tables in a loop, asking "need anything? need anything?" Constant running, mostly wasted effort.
- **Event-driven I/O**: One host at a host stand with a buzzer system. Customers press a button (event) when they need something, and the host dispatches the single waiter to the right table.

### The Evolution of Multiplexing

| System Call | Year | Max FDs | Lookup Complexity | Key Limitation |
|------------|------|---------|-------------------|----------------|
| **select()** | 1983 (4.2BSD) | 1024 (`FD_SETSIZE`) | O(n) scan | Hardcoded limit; rebuild fd_set every call |
| **poll()** | 1987 (SVR3 Unix) | Unlimited | O(n) scan | No limit, but still O(n) |
| **epoll()** | 2002 (Linux 2.5) | Unlimited | O(1) returns only ready fds | Linux-specific |
| **kqueue** | 2000 (FreeBSD 4.1) | Unlimited | O(1) | BSD/macOS-specific |
| **io_uring** | 2019 (Linux 5.1) | Unlimited | O(1) | Asynchronous; cutting-edge, complex |

### select(): The Historical Starting Point

`select()` was the first widely-available multiplexing mechanism. It works by copying a **bitmask** (`fd_set`) to the kernel, having the kernel mark which fds are ready, and returning the modified bitmask.

```cpp
#include <sys/select.h>

void event_loop_select(int server_fd) {
    fd_set read_fds;     // fd_set is a bitmask (bitmap), each bit represents one fd
    int max_fd = server_fd;

    while (true) {
        FD_ZERO(&read_fds);             // Clear all bits
        FD_SET(server_fd, &read_fds);   // Set the bit for server_fd

        for (int fd : client_fds) {
            FD_SET(fd, &read_fds);      // Set the bit for each client fd
        }

        // How select works:
        //   1. Copy the entire fd_set to kernel space
        //   2. Kernel scans every bit, checking which fds have events
        //   3. Kernel modifies the fd_set, clearing bits for non-ready fds
        //   4. Copy the modified fd_set back to user space
        //   5. Your code scans the fd_set to find which fds are ready
        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) { perror("select"); continue; }

        // New connection
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            set_nonblocking(client_fd);
            client_fds.push_back(client_fd);
            max_fd = std::max(max_fd, client_fd);
        }

        // Client data
        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            int fd = *it;
            if (FD_ISSET(fd, &read_fds)) {
                char buf[4096];
                ssize_t n = recv(fd, buf, sizeof(buf), 0);
                if (n <= 0) {
                    close(fd);
                    it = client_fds.erase(it);
                    continue;
                }
                handle_request(fd, buf, n);
            }
            ++it;
        }
    }
}
```

#### Why select() is Limited: The FD_SETSIZE Problem

`FD_SETSIZE` is a macro defined in `<sys/select.h>`. On Linux, it defaults to **1024**. Here's why:

`fd_set` is a fixed-size bitmask — typically a `long` array of 1024 bits (128 bytes on 64-bit systems). This bitmask is allocated on the **stack** in `fd_set read_fds`. Since each bit corresponds to one file descriptor, and there are only 1024 bits, you cannot monitor file descriptors numbered 1023 or higher.

The value is hardcoded at compile time as a macro:
```c
// In /usr/include/sys/select.h (Linux)
#define __FD_SETSIZE  1024
```

You could theoretically redefine it before including `<sys/select.h>`, but you'd also need to recompile the kernel and all libraries. In practice, 1024 is the limit.

**Additional select() problems:**
1. **O(n) complexity**: Every call scans all registered fds, even if only one is ready.
2. **fd_set is rebuilt every call**: You must call `FD_ZERO` and `FD_SET` before every `select()`, because the kernel modifies the bitmask in place.
3. **No event types**: `select()` only tells you "readable," "writable," or "exception" — not *what* event triggered.
4. **Thread safety**: The same fd_set cannot be shared between threads.

### poll(): Removing the Limit, Keeping the Scan

`poll()` replaces the bitmask with an array of `struct pollfd`, removing the 1024 limit. But it still requires scanning the entire array on every call — O(n) where n is the total number of fds.

```cpp
struct pollfd {
    int fd;         // file descriptor to monitor
    short events;   // events to watch for (POLLIN, POLLOUT, etc.)
    short revents;  // events that actually occurred (filled by poll())
};
```

### epoll(): The Linux Revolution

**epoll** (Event Poll) fundamentally changed the game with a key insight: **register once, monitor forever.**

Instead of copying a bitmask to the kernel on every call, epoll uses three system calls:

1. **`epoll_create1(0)`** — Create an epoll instance (returns an fd for the epoll controller)
2. **`epoll_ctl(EPOLL_CTL_ADD, fd, events)`** — Register a file descriptor with the epoll instance (this happens once, when the fd is first added)
3. **`epoll_wait(events, max, timeout)`** — Block until one or more registered fds are ready, then return only the ready ones

```mermaid
flowchart TD
    A[Event Loop] --> B{epoll_wait}
    B -->|Event ready| C{Is it server_fd?}
    B -->|Timeout| D[process_timers]

    C -->|Yes| E[accept new connection]
    C -->|No| F[recv client data]

    E --> G[add_fd to epoll]
    G --> B

    F --> H{Data read completely?}
    H -->|Yes| I[close fd]
    H -->|No| J[handle_data]
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
        // epoll_create1(0) replaces the old epoll_create(size)
        // Parameter 0 = use default behavior
        epoll_fd_ = epoll_create1(0);
    }

    void add_fd(int fd, uint32_t events) {
        struct epoll_event ev{};
        ev.events = events;     // EPOLLIN = readable, EPOLLOUT = writable
        ev.data.fd = fd;        // User data: kernel returns this unchanged
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    }

    void run(int server_fd) {
        add_fd(server_fd, EPOLLIN);

        struct epoll_event events[MAX_EVENTS];

        while (true) {
            // epoll_wait: returns only ready fds (O(1), not O(n))
            // Last parameter is timeout in ms; -1 = wait forever
            int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;

                if (fd == server_fd) {
                    // Accept all pending connections in a loop
                    while (true) {
                        int client = accept4(server_fd, nullptr, nullptr,
                                            SOCK_NONBLOCK);  // accept + set non-blocking in one call
                        if (client < 0) break;
                        add_fd(client, EPOLLIN | EPOLLET);
                    }
                } else {
                    // Read all available data from the client
                    char buf[4096];
                    while (true) {
                        ssize_t n = recv(fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            handle_data(fd, buf, n);
                        } else if (n == 0) {
                            close(fd);
                            break;
                        } else if (errno == EAGAIN) {
                            break;  // All data read
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

**Why epoll is O(1)**: The kernel maintains a **ready list** internally. When a file descriptor's state changes (e.g., data arrives on a socket), the kernel adds it to the ready list. `epoll_wait()` simply returns whatever is on the ready list — no scanning required. For a server with 10,000 connections where only 5 are active, `select()` checks all 10,000 every time, while `epoll_wait()` returns only the 5.

### kqueue: The BSD Equivalent

**kqueue** (kernel queue) is epoll's counterpart on BSD systems (FreeBSD, OpenBSD, NetBSD, and macOS). It was introduced in FreeBSD 4.1 in 2000 — two years before epoll. kqueue uses a similar register-once-monitor-forever model, but with a unified API for all event types (not just I/O — also file changes, signals, process events). If you're targeting macOS, kqueue is the native choice.

### LT vs ET: Two Trigger Modes

epoll supports two notification modes. Understanding the difference is critical for high-performance network programming.

| Mode | Name | Behavior | When to Use |
|------|------|----------|-------------|
| **LT** | Level-Triggered (default) | Notifies you whenever the buffer has data, even if you didn't read it all last time | Simple and safe — won't miss events |
| **ET** | Edge-Triggered | Notifies you only once per state change (from "no data" to "has data") | High performance, fewer syscalls, but more complex |

**The water tank analogy:**
- **LT (Level-Triggered)**: A float sensor that alarms whenever the water level is above 0. Even if you don't empty the tank completely, it keeps alarming until the tank is empty. Safe — you never miss water.
- **ET (Edge-Triggered)**: A flow sensor that only alarms when water *starts* flowing in. If you don't empty the tank completely on the first alarm, the remaining water is invisible until new water arrives. You must drain the tank completely on each alarm.

**The ET trap:**

```
Scenario: recv() reads 100 bytes, but the socket buffer still has 200 bytes

LT: Next epoll_wait still notifies ── ✅ Safe
ET: No more notifications! ── ❌ 200 bytes unread forever (until new data arrives)
```

In ET mode, you **must** read in a loop until `errno == EAGAIN`:

```cpp
// ET mode: drain the buffer completely
while (true) {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0) {
        handle_data(fd, buf, n);
    } else if (n == 0) {
        close(fd);  // Client disconnected
        break;
    } else if (errno == EAGAIN) {
        break;  // Buffer fully drained
    } else {
        close(fd);  // Error
        break;
    }
}
```

---

## 7. HTTP/1.1 Protocol Deep Dive

### What is HTTP?

**HTTP** (HyperText Transfer Protocol) is a text-based, request-response application layer protocol. It runs on top of TCP (or QUIC for HTTP/3) and is the foundation of the World Wide Web.

Key characteristics:
- **Text-based**: Requests and responses are human-readable strings (unlike binary protocols like gRPC)
- **Request-response**: The client initiates every interaction; the server never sends data without being asked (except with server push in HTTP/2)
- **Stateless**: Each request is independent; the server doesn't remember previous requests (state must be managed via cookies, tokens, or session stores)

### HTTP Request Anatomy

```
POST /api/v1/search HTTP/1.1\r\n           ← Request line: method  path  version
Host: localhost:8080\r\n                    ← Headers (key: value pairs)
Content-Type: application/json\r\n
Content-Length: 45\r\n
Authorization: Bearer token123\r\n
\r\n                                        ← Empty line = end of headers
{"query": [0.1, 0.2, 0.3], "top_k": 10}    ← Body (length = Content-Length bytes)
```

Terminology:
- **Method**: What the client wants to do. `GET` (read), `POST` (create), `PUT` (replace), `PATCH` (update), `DELETE` (remove).
- **Path/URI**: The resource identifier. `/api/v1/search` identifies the search endpoint.
- **Version**: `HTTP/1.1` — tells the server which protocol version the client supports.
- **Headers**: Key-value metadata. `Content-Type` says the body is JSON. `Content-Length` says the body is 45 bytes. `Authorization` carries the authentication token.
- **Body**: Arbitrary data payload. GET requests have no body. POST/PUT requests typically carry data.

### HTTP Response Anatomy

```
HTTP/1.1 200 OK\r\n                         ← Status line: version  status code  reason phrase
Content-Type: application/json\r\n          ← Headers
Content-Length: 62\r\n
Connection: keep-alive\r\n
\r\n                                        ← Empty line = end of headers
{"results": [{"id": 42, "score": 0.95}]}   ← Body
```

Terminology:
- **Status Code**: A three-digit integer indicating the result. More on this below.
- **Reason Phrase**: A human-readable description of the status code. Browsers ignore this; humans read it in logs.
- **Connection: keep-alive**: Tells the client the server supports persistent connections (HTTP/1.1 default).

### Status Codes: The Language of Results

Status codes are standardized by **IANA** (Internet Assigned Numbers Authority) in RFC 7231. They form a hierarchy:

```
1xx Informational
  100 Continue          Server received request headers; client should send the body
  101 Switching Protocols  Server agrees to upgrade (e.g., to WebSocket)

2xx Success
  200 OK                Request succeeded
  201 Created           New resource created (POST response)
  204 No Content        Success, but no body to return (DELETE response)

3xx Redirection
  301 Moved Permanently Resource permanently moved to a new URL
  304 Not Modified      Cached response is still valid (saves bandwidth)

4xx Client Error
  400 Bad Request       Malformed request (bad JSON, missing required field)
  401 Unauthorized      No authentication credentials provided
  403 Forbidden         Authenticated but not authorized for this resource
  404 Not Found         Resource doesn't exist at this URI
  405 Method Not Allowed  HTTP method not supported (POST to a read-only endpoint)
  422 Unprocessable Entity  Syntactically valid but semantically wrong (vector dimension mismatch)
  429 Too Many Requests   Rate limit exceeded

5xx Server Error
  500 Internal Server Error  Something broke on the server
  502 Bad Gateway           Upstream server returned an invalid response
  503 Service Unavailable   Server is temporarily overloaded or down for maintenance
```

### Keep-Alive: Why RTT Matters

**Keep-Alive** (persistent connections) lets multiple HTTP requests share a single TCP connection. Without it, every request pays the 1.5 RTT cost of the TCP handshake.

Why is RTT (Round-Trip Time) so expensive? Because TCP's three-way handshake requires a full round trip before data can flow:

1. Client → Server: `SYN` ("I want to connect")
2. Server → Client: `SYN-ACK` ("OK, I'm ready")
3. Client → Server: `ACK` ("Good, here's my HTTP request")

Only after step 3 does the actual HTTP request data start flowing. The `ACK` and the first byte of the HTTP request can be sent together (piggybacked), saving one more half-RTT. So the minimum overhead is **1 RTT** for connection setup.

On a typical broadband connection (20ms RTT), that's 20ms wasted per request. On a mobile connection (100ms RTT), it's 100ms. Multiply by 50 sub-resources per page, and you've got 5 seconds of pure handshake overhead. Keep-alive eliminates this by reusing connections.

### Content-Length vs Transfer-Encoding: chunked

These are two ways to tell the receiver "how much body data to expect":

- **Content-Length**: The exact byte count is known before sending begins. Used for most responses.
- **Transfer-Encoding: chunked**: The body is sent in chunks, each prefixed with its hex size. Used when the server generates data incrementally (streaming, SSE, large file transfers).

```
HTTP/1.1 200 OK\r\n
Transfer-Encoding: chunked\r\n
\r\n
1A\r\n                    ← Hex 1A = 26 bytes
{"results": [{"id": 1}   ← Chunk 1 (26 bytes)
\r\n
0D\r\n                    ← Hex 0D = 13 bytes
, {"id": 2}]}\r\n        ← Chunk 2 (13 bytes)
\r\n
0\r\n                     ← Size 0 = end of chunks
\r\n

```

```cpp
std::string encode_chunked(const std::string& data) {
    std::stringstream ss;
    ss << std::hex << data.size() << "\r\n";
    ss << data << "\r\n";
    return ss.str();
}
```

### Headers: The Metadata Layer

Headers carry everything that isn't the body. Key headers to know:

| Header | Purpose | Example |
|--------|---------|---------|
| `Host` | Which domain this request is for (required in HTTP/1.1) | `Host: example.com` |
| `Content-Type` | MIME type of the body | `Content-Type: application/json` |
| `Content-Length` | Byte length of the body | `Content-Length: 45` |
| `Authorization` | Authentication credentials | `Authorization: Bearer sk-...` |
| `Accept` | What content types the client accepts | `Accept: application/json` |
| `Connection` | Connection management | `Connection: keep-alive` or `Connection: close` |
| `Cache-Control` | Caching instructions | `Cache-Control: no-cache` |
| `User-Agent` | Client software identification | `User-Agent: curl/7.68.0` |

### URI vs URL vs URN

These terms are often confused:

- **URI** (Uniform Resource Identifier): The broadest term. Any string that identifies a resource. `/api/v1/health` is a URI.
- **URL** (Uniform Resource Locator): A URI that includes *how to access* the resource — scheme, host, path. `http://localhost:8080/api/health` is a URL.
- **URN** (Uniform Resource Name): A URI that names a resource without specifying location. `urn:isbn:0451450523` is a URN.

All URLs are URIs, but not all URIs are URLs. In practice, people say "URL" when they mean "URI."

### Query Parameters

**Query parameters** are key-value pairs appended to the URI after a `?`:

```
GET /api/v1/search?q=hello&top_k=10&threshold=0.8 HTTP/1.1
     ─────── ─────────────────────────────────
     path    query string (everything after ?)
```

The query string is parsed by the server: `q=hello`, `top_k=10`, `threshold=0.8`. Multiple values for the same key are allowed: `?color=red&color=blue`.

### JSON: The Universal Exchange Format

**JSON** (JavaScript Object Notation) is a lightweight text format for structured data. It's the lingua franca of web APIs.

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

JSON maps directly to data structures in most languages:
- **Object** `{}` → C++ `std::map<string, T>` or struct
- **Array** `[]` → C++ `std::vector<T>`
- **String** `""` → C++ `std::string`
- **Number** → C++ `int` or `double`
- **Boolean** → C++ `bool`
- **null** → `nullptr` or `std::nullopt`

**Serialization** is the process of converting in-memory objects to JSON strings (and back). The C++ library `nlohmann/json` is the de facto standard: `json j = {{"key", "value"}}; std::string s = j.dump();` and `json parsed = json::parse(s);`.

---

## 8. REST API Design

### What is REST?

**REST** (REpresentational State Transfer) is an architectural style described by **Roy Fielding** in his 2000 PhD dissertation. Fielding was one of the principal authors of HTTP/1.1, and REST formalized the architectural principles that made the web work.

REST's core constraints:

1. **Resources identified by URIs**: Every entity has a unique identifier. `/api/v1/vectors/42` is the vector with ID 42.
2. **Uniform interface**: Operations use HTTP methods — GET (read), POST (create), PUT (replace), PATCH (update), DELETE (remove).
3. **Stateless**: Each request contains all information the server needs. The server doesn't remember "the previous request." This makes horizontal scaling trivial — any server can handle any request.
4. **Representation**: Resources are transmitted as representations (JSON, XML, etc.). The same resource can have multiple representations.
5. **Layered system**: Intermediaries (load balancers, caches, proxies) can sit between client and server without the client knowing.

### DeepVector's RESTful Routes

```
POST   /api/v1/vectors         Insert a vector
GET    /api/v1/vectors/{id}    Retrieve a vector
DELETE /api/v1/vectors/{id}    Delete a vector
POST   /api/v1/search          Similarity search
GET    /api/v1/health          Health check
GET    /api/v1/stats           Statistics
```

The design follows REST conventions:
- **Nouns, not verbs** in URLs: `/vectors`, not `/getVectors`
- **Plural nouns**: `/vectors`, not `/vector`
- **Version prefix**: `/api/v1/` so you can introduce breaking changes in `/api/v2/` without breaking existing clients
- **HTTP methods encode intent**: `GET /vectors/42` means "read vector 42", not "search for vectors"

### Unified JSON Response Format

```json
// Success
{
    "status": "ok",
    "data": { "id": 42, "results": [{"id": 7, "score": 0.95}] }
}

// Error
{
    "status": "error",
    "error": {
        "code": "DIMENSION_MISMATCH",
        "message": "Expected dimension 768, got 1024"
    }
}
```

This uniform format lets clients parse responses consistently. Every success has a `"status": "ok"` and `"data"`. Every error has `"status": "error"` and `"error"` with a machine-readable `code` and human-readable `message`.

### Router: The Traffic Cop

A **router** (in the HTTP framework sense) maps incoming requests to handler functions. It examines the method and path, then dispatches to the correct handler:

```cpp
class Router {
    // Key: "GET /api/v1/health", Value: handler function
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

### CORS: Sharing Resources Across Origins

**CORS** (Cross-Origin Resource Sharing) is a security mechanism that controls which web pages can request resources from different domains.

The **same-origin policy** is the browser's default security model: a web page at `http://app.example.com` can only make HTTP requests to `http://app.example.com`. Requests to `http://api.example.com` or `http://localhost:8080` are blocked.

CORS relaxes this via HTTP headers:

```
Access-Control-Allow-Origin: http://app.example.com
Access-Control-Allow-Methods: GET, POST, DELETE
Access-Control-Allow-Headers: Content-Type, Authorization
Access-Control-Max-Age: 86400
```

For APIs (not browser-based), CORS is irrelevant — browsers enforce it, but `curl`, mobile apps, and server-to-server calls don't. However, if your API is consumed by a web frontend, you'll need CORS headers.

---

## 9. Authentication and Authorization

### Authentication vs Authorization

- **Authentication**: Proving who you are. "Show me your ID."
- **Authorization**: Determining what you're allowed to do. "Your ID grants you access to this floor, but not that one."

Authentication comes first; authorization follows.

### Bearer Token Authentication

**Bearer Token** is the most common API authentication scheme. The client sends a token in the `Authorization` header:

```
Authorization: Bearer sk-lumen-test-token
```

"Bearer" means "whoever bears (holds) this token is treated as the authenticated user." The token is essentially a password — anyone who has it can impersonate you. This is why HTTPS is mandatory in production: without encryption, anyone sniffing the network can steal the token.

```cpp
std::string extract_token(const HttpRequest& req) {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return "";

    const std::string& header = it->second;
    if (header.substr(0, 7) != "Bearer ") return "";
    return header.substr(7);
}

bool verify_token(const std::string& token) {
    static const std::string VALID_TOKEN = "sk-lumen-test-token";
    return token == VALID_TOKEN;
}
```

### API Keys

An **API key** is a long, random string generated by the server and given to the user. It's a simpler alternative to JWT, often used for server-to-server communication. The key identifies the caller and is checked against a database or list.

### JWT: Self-Contained Tokens

**JWT** (JSON Web Token) is a self-contained, verifiable token format defined in RFC 7519. Its structure:

```
eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1c2VyMTIzIiwiZXhwIjoxNjkwMDg2NDAwfQ.abc123def456
├── Header ──────────┤├── Payload ───────────────────────────┤├── Signature ───┤
```

- **Header**: Signing algorithm (e.g., `{"alg": "HS256", "typ": "JWT"}`)
- **Payload**: Claims — user ID, expiration, permissions, any custom data
- **Signature**: HMAC or RSA signature over `base64(header) + "." + base64(payload)`, using a secret key

**Advantage**: The server doesn't need to look up the token in a database. It just verifies the signature with its secret key. Any service with the secret key can independently validate the token — perfect for distributed systems.

**Weakness**: Once issued, a JWT cannot be revoked before expiration. If a user is banned, their JWT remains valid until it expires. Solutions include short expiration times (5-15 minutes) plus a refresh token mechanism, or maintaining a server-side blocklist (which negates the stateless benefit).

### Refresh Token + Access Token Pattern

To balance security and usability:

- **Access token**: Short-lived (5-15 minutes), used for API calls
- **Refresh token**: Long-lived (days/weeks), used only to obtain new access tokens

When the access token expires, the client sends the refresh token to a dedicated endpoint to get a fresh access token. If the user is banned, the refresh token is invalidated, and the user must re-authenticate.

---

## 10. Graceful Shutdown

### Process Signals

**Signals** are the operating system's asynchronous notification mechanism for processes. They're like push notifications for programs:

| Signal | Trigger | Default Action |
|--------|---------|----------------|
| **SIGINT** | User presses Ctrl+C | Terminate the process |
| **SIGTERM** | `kill <pid>` or `docker stop` | Terminate (can be caught and handled) |
| **SIGKILL** | `kill -9 <pid>` | Forced termination (cannot be caught) |
| **SIGHUP** | Terminal hangup | Traditionally: reload config |

**SIGTERM** is "please clean up and exit." **SIGKILL** is "die immediately, no questions asked."

### Graceful Shutdown: Five Steps

**Graceful shutdown** ensures the server finishes processing in-flight requests before exiting, rather than abruptly cutting off clients:

```
1. Receive SIGTERM ──► Set a flag: g_running = false
2. close(listen_fd) ──► Stop accepting new connections
3. Drain in-flight requests ──► Let current requests finish (grace period, usually 10-30s)
4. Persist state ──► Flush indexes to disk, write WAL
5. exit(0) ──► Clean exit
```

In Docker, `docker stop` sends SIGTERM, waits 10 seconds (configurable), then sends SIGKILL. Your process **must** finish cleanup within the grace period or it will be killed ungracefully.

### Complete Implementation

```cpp
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signal_handler(int sig) {
    g_running = 0;  // Only set the flag in the signal handler; no complex operations
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int server_fd = create_server(8080);

    while (g_running) {
        // Normal event loop
        epoll_wait(...);
    }

    // Graceful shutdown begins
    printf("Shutting down gracefully...\n");

    // 1. Stop accepting new connections
    close(server_fd);

    // 2. Wait for in-flight requests (max 30s grace period)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!active_connections.empty() &&
           std::chrono::steady_clock::now() < deadline) {
        poll_remaining_connections(100);
    }

    // 3. Force-close remaining connections
    for (int fd : active_connections) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    // 4. Persist state
    index_.flush();

    printf("Shutdown complete.\n");
    return 0;
}
```

Key details:
- `sig_atomic_t` is the only type guaranteed to be read/written atomically in a signal handler. Never use regular `int` in signal handlers.
- The signal handler only sets a flag. All real work happens in the main thread.
- `shutdown(fd, SHUT_RDWR)` gracefully closes both send and receive directions of a socket.

---

## 11. Production HTTP Servers

### nginx

**nginx** (pronounced "engine-x") is the most widely deployed HTTP server, powering ~34% of all websites (W3Techs, 2025). It was created by **Igor Sysoev** in 2004 to solve the C10K problem.

Architecture:
- **Master process**: Reads config, binds ports, manages worker processes
- **Worker processes**: Handle all I/O using an event-driven (epoll/kqueue) model. Each worker handles thousands of connections simultaneously.
- **No thread-per-connection**: nginx uses a single-threaded, non-blocking event loop per worker. This eliminates locking complexity and context-switch overhead.

nginx is both a **reverse proxy** (forwarding requests to backend application servers) and a **static file server** (serving files directly from disk). Its configuration language is notoriously verbose but extremely powerful.

### Envoy

**Envoy** is a modern, high-performance L4/L7 proxy originally built at Lyft. It's the standard data plane for **service mesh** architectures (Istio, Consul Connect).

Key differences from nginx:
- Written in **C++17** (nginx is C)
- Dynamic configuration via xDS API — no restarts needed for config changes
- First-class observability (distributed tracing, metrics, logging)
- Designed for microservice-to-microservice communication, not end-user-facing serving

### Node.js

**Node.js** proved that a single-threaded, event-driven model (using libuv on Linux, which uses epoll) could build high-performance HTTP servers. Its non-blocking I/O model popularized the pattern for application developers.

### Why This Pattern Dominates

The event-driven model won because:
1. **No thread overhead**: One thread handles thousands of connections
2. **No locking**: Single-threaded = no mutexes, no race conditions, no deadlocks
3. **Predictable latency**: No context switches, no lock contention
4. **Efficient memory usage**: One stack instead of thousands

The tradeoff: code is more complex. You can't make blocking calls (database queries, file reads, DNS lookups) in the event loop — they'll block all other connections. Solutions include thread pools, asynchronous I/O, and non-blocking libraries.

---

## 12. Discussion Questions

1. Why is `select()`'s `FD_SETSIZE` limited to 1024? Is this a technical constraint or a design choice? (Hint: research the `FD_SETSIZE` macro, `__FD_SETSIZE`, and how the bitmask is allocated on the stack.)

2. In epoll's ET mode, if you break out of the `recv` loop before reaching `EAGAIN`, when will the remaining data be processed? Why is this a "silent data loss"?

3. What is the fundamental difference between HTTP/1.1's `Connection: keep-alive` and HTTP/2's multiplexing? (Hint: at which layer does head-of-line blocking occur?)

4. If a client sends a `Content-Length` that doesn't match the actual body length (intentionally or by accident), how should the server defend itself? List all possible attack scenarios.

5. Design a sliding window rate limiter: how do you limit to 100 search requests per minute? How does the counter recover when the server restarts?

6. How does stateless JWT authentication handle "user is banned but token hasn't expired"? Can a refresh token + access token dual-token mechanism solve this?

7. What problem does `SO_REUSEADDR` solve vs `SO_REUSEPORT`? In a multi-process model (like nginx workers), how does `SO_REUSEPORT` enable kernel-level load balancing?

8. After `close(fd)`, the fd number is immediately reused by `accept()`. Could the new connection accidentally read stale data from the old connection? How does the TCP protocol stack prevent data mixing?

---

## 13. Exercises

### Exercise 1: Single-Threaded epoll HTTP Server (35 min)

Build an HTTP server from scratch using epoll (ET mode), supporting:
- `GET /health` → `200 OK {"status":"ok"}`
- `POST /echo` → returns the request body
- Manual HTTP request parsing (no HTTP libraries)

### Exercise 2: Full REST API (30 min)

Implement CRUD REST API for the vector database:
- `POST /vectors` — insert a vector (`{"id": 1, "vector": [...]}`)
- `GET /vectors/{id}` — retrieve a vector
- `POST /search` — KNN search (`{"vector": [...], "k": 10}`)
- `DELETE /vectors/{id}` — delete a vector

### Exercise 3: Authentication Middleware (20 min)

Implement Bearer token authentication middleware that intercepts all `/api/v1/*` requests, validates the `Authorization: Bearer <token>` header, returns 401 if missing, 403 if invalid. Replace the fixed token with JWT (using OpenSSL HMAC-SHA256).

### Exercise 4: Graceful Shutdown + Keep-Alive (25 min)

Add to your server:
1. Keep-Alive support (multiple HTTP requests per connection)
2. SIGTERM handling (stop accept → drain requests → set `Connection: close` to notify clients → close → exit)

### Exercise 5: Stress Testing (20 min)

Use `wrk` to stress test your server:
```bash
wrk -t 4 -c 100 -d 30s --latency http://localhost:8080/api/v1/health
```
Report QPS, P50/P99 latency. Analyze the bottleneck: parsing? I/O? Search?

---

## Chapter Summary

| Layer | Core Technology | Key Concepts |
|-------|-----------------|--------------|
| **Transport** | Berkeley Sockets API | fd, socket() → bind() → listen() → accept() → send()/recv() → close() |
| **Multiplexing** | select → poll → epoll/kqueue | C10K problem; LT vs ET; O(1) event notification |
| **Protocol** | HTTP/1.1 | Request line + headers + body; Keep-Alive; Content-Length vs chunked |
| **Application** | REST API | Resource URIs + HTTP methods (CRUD); unified JSON responses; status codes |
| **Security** | Authentication | Bearer Token; JWT three-part structure; authentication vs authorization |
| **Operations** | Signal handling | SIGTERM → drain requests → persist → exit; grace period |

> Next chapter: [Chapter 11: C++20 Coroutines & SkyNet](../ch11_coroutines/README.md)
