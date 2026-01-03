# WebSocket Mastery - From First Principles to Production

**Project Goal:** Master WebSocket protocol from ground-up implementation to production-ready real-time systems.

---

## **[Mind Map] - WebSocket Architecture**

```
WebSocket Mastery
│
├── Protocol Layer
│   ├── HTTP Upgrade Handshake (101 Switching Protocols)
│   ├── Frame Structure (Opcode, Payload, Masking)
│   └── Control Frames (Ping/Pong, Close)
│
├── Connection Management
│   ├── Connection Lifecycle (Open → Message → Close)
│   ├── Heartbeat/Keep-alive
│   └── Reconnection Logic
│
├── Concurrency Model
│   ├── Event Loop (async I/O)
│   ├── Thread Pool for handlers
│   └── Lock-free message queues
│
└── System Design Patterns
    ├── Pub/Sub with Channels
    ├── Horizontal Scaling (Redis/Message Broker)
    ├── Load Balancing (Sticky Sessions)
    └── Backpressure & Flow Control
```

---

## **[The Basics] - WebSocket First Principles**

### **1. Why WebSockets?**
- **Problem:** HTTP is half-duplex (request-response). Real-time apps need full-duplex bidirectional communication.
- **Solution:** Persistent TCP connection with minimal framing overhead (~2 bytes vs HTTP headers).
- **Performance:** Eliminates HTTP header overhead (avg 800-1000 bytes) per message
- **Latency:** No connection establishment per message (saves TCP handshake + TLS handshake)

### **2. The Handshake (HTTP → WebSocket Upgrade)**

**Client Request:**
```http
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
Origin: http://example.com
```

**Server Response:**
```http
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

**Handshake Logic:**
- **Sec-WebSocket-Key:** Random 16-byte value, base64 encoded (prevents cache poisoning)
- **Sec-WebSocket-Accept:** `base64(SHA-1(Sec-WebSocket-Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))`
- **Magic String:** RFC 6455 GUID, proves server understands WebSocket protocol

### **3. Frame Structure (RFC 6455)**

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```

**Frame Fields:**
- **FIN (1 bit):** Final fragment flag (1 = complete message, 0 = fragmented)
- **RSV1-3 (3 bits):** Reserved for extensions (must be 0 unless negotiated)
- **Opcode (4 bits):**
  - `0x0`: Continuation frame
  - `0x1`: Text frame (UTF-8)
  - `0x2`: Binary frame
  - `0x8`: Close connection
  - `0x9`: Ping
  - `0xA`: Pong
- **MASK (1 bit):** 1 if payload is masked (required for client → server)
- **Payload Length (7 bits, 7+16, or 7+64):**
  - 0-125: Actual length
  - 126: Next 16 bits = length
  - 127: Next 64 bits = length
- **Masking Key (32 bits):** XOR key for payload (only if MASK=1)

**Masking Algorithm:**
```c
for (size_t i = 0; i < payload_len; i++) {
    decoded[i] = encoded[i] ^ masking_key[i % 4];
}
```

### **4. Connection Lifecycle**

```
┌─────────────┐
│   CONNECTING │
└──────┬──────┘
       │ HTTP Upgrade Success
       ▼
┌─────────────┐
│     OPEN    │ ◄──┐
└──────┬──────┘    │ Ping/Pong
       │           │ Data Frames
       │           └───────────┐
       │ Close Frame           │
       ▼                       │
┌─────────────┐                │
│   CLOSING   │                │
└──────┬──────┘                │
       │ Close ACK             │
       ▼                       │
┌─────────────┐                │
│    CLOSED   │                │
└─────────────┘                │
                               │
       Network Error ──────────┘
```

---

## **[Mastery] - Progressive Implementation Path**

### **Phase 1: Raw WebSocket Server (C++)**

**Goal:** Build protocol parser and event loop from scratch.

**Architecture:**
```
┌──────────────────────────────────────────────────────┐
│              Application Layer                        │
│  (Message Handlers, Business Logic, Pub/Sub)         │
└───────────────────┬──────────────────────────────────┘
                    │
┌───────────────────▼──────────────────────────────────┐
│            WebSocket Protocol Layer                   │
│  • Frame Parser (State Machine)                       │
│  • Frame Builder (Masking, Fragmentation)             │
│  • Handshake Handler                                  │
└───────────────────┬──────────────────────────────────┘
                    │
┌───────────────────▼──────────────────────────────────┐
│              Event Loop Layer                         │
│  • epoll/kqueue (I/O Multiplexing)                    │
│  • Non-blocking Socket I/O                            │
│  • Timer Management (Ping/Timeout)                    │
└───────────────────┬──────────────────────────────────┘
                    │
┌───────────────────▼──────────────────────────────────┐
│               TCP Socket Layer                        │
│  • Socket Options (TCP_NODELAY, SO_REUSEADDR)         │
│  • Buffer Management                                  │
└───────────────────────────────────────────────────────┘
```

**Key Components:**

**1. Event Loop (macOS: kqueue)**
```cpp
class EventLoop {
    int kqueue_fd_;
    std::unordered_map<int, Connection*> connections_;
    
public:
    void run() {
        struct kevent events[MAX_EVENTS];
        while (running_) {
            int n = kevent(kqueue_fd_, nullptr, 0, events, MAX_EVENTS, nullptr);
            for (int i = 0; i < n; i++) {
                handle_event(events[i]);
            }
        }
    }
};
```

**2. Frame Parser (State Machine)**
```cpp
enum class ParseState {
    HEADER,
    PAYLOAD_LENGTH_16,
    PAYLOAD_LENGTH_64,
    MASKING_KEY,
    PAYLOAD,
    COMPLETE
};

class FrameParser {
    ParseState state_ = ParseState::HEADER;
    Frame current_frame_;
    
    ParseResult parse(const uint8_t* data, size_t len);
};
```

**3. Connection Manager**
```cpp
class Connection {
    int fd_;
    std::queue<Frame> send_queue_;
    FrameParser parser_;
    HandshakeState handshake_state_;
    
    void on_readable();
    void on_writable();
    void send_frame(const Frame& frame);
};
```

**Implementation Checklist:**
- [ ] Socket creation and binding
- [ ] kqueue event loop setup
- [ ] HTTP handshake parser and validator
- [ ] WebSocket frame parser (handle fragmentation)
- [ ] Frame sender (handle EWOULDBLOCK)
- [ ] Ping/Pong heartbeat mechanism
- [ ] Graceful close handshake
- [ ] Connection timeout handling

---

### **Phase 2: Real-Time Chat System**

**System Architecture:**
```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Browser   │     │   Browser   │     │   Browser   │
│  (Client)   │     │  (Client)   │     │  (Client)   │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                   │
       │ WebSocket         │ WebSocket         │ WebSocket
       │                   │                   │
       ▼                   ▼                   ▼
┌─────────────────────────────────────────────────────┐
│            WebSocket Server (C++/Node.js)            │
│  • Connection Pool Management                        │
│  • Channel/Room Management                           │
│  • Message Broadcasting                              │
└───────────────────┬─────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        │           │           │
        ▼           ▼           ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│  Redis   │  │PostgreSQL│  │  Redis   │
│ Pub/Sub  │  │ Messages │  │  Cache   │
└──────────┘  └──────────┘  └──────────┘
```

**Features to Implement:**
1. **Channel/Room System**
   - Users join/leave channels
   - Message broadcast within channel
   - User presence tracking

2. **Message Types**
   ```json
   // Join Channel
   {"type": "join", "channel": "general", "user": "alice"}
   
   // Send Message
   {"type": "message", "channel": "general", "text": "Hello!"}
   
   // User Presence
   {"type": "presence", "channel": "general", "users": ["alice", "bob"]}
   ```

3. **Persistence Layer**
   - Store messages in PostgreSQL
   - Cache recent messages in Redis
   - Query message history on join

4. **Horizontal Scaling**
   - Multiple server instances
   - Redis Pub/Sub for cross-server messaging
   - Sticky sessions in load balancer

**Data Structures:**
```cpp
// In-memory channel management
class ChannelManager {
    std::unordered_map<std::string, Channel> channels_;
    std::shared_mutex mutex_;
    
public:
    void join(const std::string& channel, Connection* conn);
    void leave(const std::string& channel, Connection* conn);
    void broadcast(const std::string& channel, const std::string& msg);
};

class Channel {
    std::string name_;
    std::unordered_set<Connection*> subscribers_;
    std::deque<Message> history_; // Last N messages
};
```

---

### **Phase 3: Advanced Patterns & Optimization**

#### **1. Performance Optimization**

**Zero-Copy I/O:**
```cpp
// Use scatter-gather I/O for frames
struct iovec iov[2];
iov[0].iov_base = &frame_header;
iov[0].iov_len = sizeof(frame_header);
iov[1].iov_base = payload;
iov[1].iov_len = payload_len;
writev(socket_fd, iov, 2);
```

**Memory Pooling:**
```cpp
// Pre-allocate frame buffers to avoid malloc churn
class FramePool {
    std::vector<Frame*> free_frames_;
    std::mutex mutex_;
    
public:
    Frame* acquire() {
        std::lock_guard lock(mutex_);
        if (free_frames_.empty()) {
            return new Frame();
        }
        Frame* frame = free_frames_.back();
        free_frames_.pop_back();
        return frame;
    }
    
    void release(Frame* frame) {
        frame->reset();
        std::lock_guard lock(mutex_);
        free_frames_.push_back(frame);
    }
};
```

**Lock-Free Queues:**
```cpp
// Use lock-free queue for message passing between threads
#include <boost/lockfree/queue.hpp>

boost::lockfree::queue<Message*> message_queue_(1024);
```

#### **2. Reliability Patterns**

**Backpressure Handling:**
```cpp
class Connection {
    static constexpr size_t MAX_QUEUE_SIZE = 100;
    std::queue<Frame> send_queue_;
    
    bool send_frame(const Frame& frame) {
        if (send_queue_.size() >= MAX_QUEUE_SIZE) {
            // Option 1: Drop oldest message
            send_queue_.pop();
            
            // Option 2: Close connection (prevent memory exhaustion)
            // close_connection();
            // return false;
        }
        send_queue_.push(frame);
        return true;
    }
};
```

**Circuit Breaker for Backend Services:**
```cpp
class CircuitBreaker {
    enum State { CLOSED, OPEN, HALF_OPEN };
    State state_ = CLOSED;
    int failure_count_ = 0;
    std::chrono::steady_clock::time_point last_failure_;
    
public:
    bool allow_request() {
        if (state_ == OPEN) {
            auto now = std::chrono::steady_clock::now();
            if (now - last_failure_ > std::chrono::seconds(60)) {
                state_ = HALF_OPEN;
                return true;
            }
            return false;
        }
        return true;
    }
    
    void on_success() {
        failure_count_ = 0;
        state_ = CLOSED;
    }
    
    void on_failure() {
        failure_count_++;
        last_failure_ = std::chrono::steady_clock::now();
        if (failure_count_ >= 5) {
            state_ = OPEN;
        }
    }
};
```

**Heartbeat with Timeout:**
```cpp
class Connection {
    std::chrono::steady_clock::time_point last_pong_;
    
    void start_heartbeat_timer() {
        // Send ping every 30s
        // Close connection if no pong within 60s
    }
    
    void on_pong_received() {
        last_pong_ = std::chrono::steady_clock::now();
    }
    
    void check_heartbeat() {
        auto now = std::chrono::steady_clock::now();
        if (now - last_pong_ > std::chrono::seconds(60)) {
            close_connection();
        }
    }
};
```

#### **3. Protocol Enhancements**

**Compression (permessage-deflate extension):**
```cpp
// Negotiate during handshake
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits

// Compress payload with zlib
z_stream zs;
deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
deflate(&zs, Z_SYNC_FLUSH);
```

**Binary Protocol with Protobuf:**
```protobuf
message ChatMessage {
    string channel = 1;
    string user = 2;
    string text = 3;
    int64 timestamp = 4;
}
```

**Multiplexing Channels (Custom Extension):**
```cpp
// Frame format: [channel_id (4 bytes)][payload]
struct MultiplexFrame {
    uint32_t channel_id;
    std::vector<uint8_t> payload;
};
```

---

## **[Pro-Tip] - System-Level Insights**

### **1. TCP Tuning for WebSockets**

**Disable Nagle's Algorithm:**
```cpp
int flag = 1;
setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
// Benefit: Reduces latency for small messages
// Trade-off: More packets on network (acceptable for real-time apps)
```

**Increase Socket Buffers:**
```cpp
int send_buffer = 256 * 1024; // 256KB
int recv_buffer = 256 * 1024;
setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &send_buffer, sizeof(send_buffer));
setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &recv_buffer, sizeof(recv_buffer));
// Benefit: Reduces system call overhead, improves throughput
```

**Enable TCP Keep-Alive:**
```cpp
int keepalive = 1;
int keepidle = 60;  // Start probing after 60s idle
int keepintvl = 10; // Probe every 10s
int keepcnt = 3;    // Drop after 3 failed probes
setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
```

### **2. Memory Management**

**Connection Memory Footprint:**
```
Per Connection = 
    File Descriptor:        8 bytes
    Send Buffer:            256 KB (tunable)
    Recv Buffer:            256 KB (tunable)
    Send Queue:             ~100 frames × ~1KB = 100 KB
    Parser State:           ~1 KB
    User Data:              ~1 KB
    ────────────────────────────────
    Total:                  ~614 KB per connection

10K Connections:            ~6 GB RAM
100K Connections:           ~60 GB RAM
```

**Strategies:**
- Use memory pools for fixed-size allocations
- Implement memory limits per connection
- Monitor RSS and trigger GC/cleanup when threshold exceeded
- Consider using `jemalloc` or `tcmalloc` for better allocator performance

### **3. Concurrency Models**

**Model A: Thread-per-Core (Recommended for C++)**
```
┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐
│Core 0  │  │Core 1  │  │Core 2  │  │Core 3  │
│Thread 0│  │Thread 1│  │Thread 2│  │Thread 3│
│kqueue  │  │kqueue  │  │kqueue  │  │kqueue  │
│5K conn │  │5K conn │  │5K conn │  │5K conn │
└────────┘  └────────┘  └────────┘  └────────┘
```
- Pin threads to CPU cores
- Each thread has own event loop
- Use `SO_REUSEPORT` to distribute incoming connections
- Zero lock contention for connection I/O

**Model B: Event Loop + Thread Pool**
```
┌──────────────────┐
│   Event Loop     │ (Single thread, handles I/O)
│   (kqueue)       │
└────────┬─────────┘
         │ Dispatch work
         ▼
┌────────────────────────────────┐
│       Thread Pool               │
│  [Worker] [Worker] [Worker]    │
│  (Process messages)             │
└────────────────────────────────┘
```
- Event loop only handles I/O (fast path)
- Offload CPU-intensive tasks to thread pool
- Use lock-free queues for task passing

### **4. Load Testing Benchmarks**

**Tools:**
- `wscat` - Simple CLI WebSocket client
- `websocat` - Swiss-army knife for WebSocket
- `bombardier` - HTTP/WS load testing tool
- Custom load test with `libwebsockets`

**Metrics to Track:**
- **Connection Rate:** Connections/sec
- **Message Rate:** Messages/sec
- **Latency:** p50, p95, p99 (use HDR Histogram)
- **Throughput:** MB/sec
- **CPU Usage:** Per core utilization
- **Memory:** RSS, heap allocations

**Target Performance (Single Server):**
- 50K+ concurrent connections
- 100K+ messages/sec
- <10ms p99 latency (small messages)
- <5% CPU per 10K connections

### **5. Error Handling Edge Cases**

**Half-Closed Connections:**
```cpp
// Client sends FIN but server still has data to send
if (recv(fd, buf, len, 0) == 0) {
    // Client closed read-side, but might still accept writes
    // Flush send queue, then close
}
```

**Partial Frame Reads:**
```cpp
// Frame might arrive over multiple recv() calls
// Parser must maintain state across reads
ParseResult FrameParser::parse(const uint8_t* data, size_t len) {
    while (offset < len) {
        switch (state_) {
            case HEADER:
                if (buffer_.size() < 2) {
                    buffer_.append(data + offset, std::min(len - offset, 2 - buffer_.size()));
                    if (buffer_.size() < 2) return INCOMPLETE;
                }
                // Parse header...
                break;
            // ... other states
        }
    }
}
```

**Malformed Frames:**
```cpp
// Always validate before processing
if (!validate_frame(frame)) {
    send_close_frame(1002, "Protocol error");
    return;
}

// Common violations:
// - Client sends unmasked frame
// - Reserved bits set when no extension negotiated
// - Invalid UTF-8 in text frame
// - Control frame with payload > 125 bytes
```

---

## **Project Roadmap**

### **Day 1: Foundation**
- [ ] Setup project structure (CMakeLists.txt, Makefile)
- [ ] Implement TCP socket with kqueue event loop
- [ ] HTTP request parser for handshake detection
- [ ] WebSocket handshake response generator
- [ ] Basic echo server (accept one connection)

### **Day 2: Protocol Implementation**
- [ ] Frame parser state machine
- [ ] Handle fragmented frames
- [ ] Frame sender with masking
- [ ] Implement ping/pong heartbeat
- [ ] Graceful close handshake
- [ ] Test with browser client

### **Day 3: Chat System**
- [ ] Channel/Room management
- [ ] Message broadcasting
- [ ] User presence tracking
- [ ] Simple web UI (HTML/JS)

### **Day 4: Scaling & Polish**
- [ ] Redis Pub/Sub integration
- [ ] PostgreSQL message persistence
- [ ] Load balancer configuration
- [ ] Performance testing
- [ ] Memory leak detection (valgrind)

---

## **References**

**RFCs:**
- [RFC 6455](https://tools.ietf.org/html/rfc6455) - The WebSocket Protocol
- [RFC 7692](https://tools.ietf.org/html/rfc7692) - Compression Extensions

**Libraries for Comparison:**
- **C++:** `libwebsockets`, `uWebSockets`, `Boost.Beast`
- **Node.js:** `ws`, `socket.io`
- **Go:** `gorilla/websocket`, `gobwas/ws`

**System Programming:**
- [The C10K Problem](http://www.kegel.com/c10k.html)
- [epoll vs kqueue](https://people.eecs.berkeley.edu/~sangjin/2012/12/21/epoll-vs-kqueue.html)

**Load Testing:**
- [How Discord handles 10M+ concurrent connections](https://discord.com/blog/how-discord-handles-push-request-bursts-of-over-a-million-per-minute)

---

## **Next Steps**

1. **Set up development environment**
   ```bash
   cd /Users/codebreaker/cbr/projects/myprojects/websocket_cpp
   mkdir -p src/{server,protocol,utils} include tests
   touch CMakeLists.txt README.md
   ```

2. **Choose implementation approach:**
   - Start with raw C++ implementation (deep learning)
   - Or use library like Boost.Beast (faster to production)

3. **Create initial project structure**

**Ready to start coding?** Let me know which phase you want to tackle first!
