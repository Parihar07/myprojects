# WebSocket Mastery Project

**From First Principles to Production-Ready Implementation**

## Quick Start (Boost.Beast)

This is the fast path to understanding WebSocket architecture using Boost.Beast library.

### Prerequisites

```bash
# macOS
brew install boost cmake

# Verify installation
brew list boost
```

### Build & Run

```bash
cd /Users/codebreaker/cbr/projects/myprojects/websocket_cpp

# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make

# Run the server
./websocket_beast_server 9001
```

### Test the Server

1. **Option A: Using Browser**
   - Open `client/test_client.html` in your browser
   - Click "Connect"
   - Type messages and see echo responses

2. **Option B: Using wscat (CLI)**
   ```bash
   # Install wscat
   npm install -g wscat
   
   # Connect and test
   wscat -c ws://localhost:9001
   > Hello WebSocket!
   < Hello WebSocket!
   ```

3. **Option C: Using curl (HTTP upgrade)**
   ```bash
   curl --include \
        --no-buffer \
        --header "Connection: Upgrade" \
        --header "Upgrade: websocket" \
        --header "Sec-WebSocket-Key: SGVsbG8sIHdvcmxkIQ==" \
        --header "Sec-WebSocket-Version: 13" \
        http://localhost:9001/
   ```

## Architecture Overview

### Boost.Beast Implementation

```
Client Browser
    │
    │ WebSocket Protocol (ws://)
    │
    ▼
┌─────────────────────────────────────────┐
│  Listener (Accept Connections)          │
│  • Binds to 0.0.0.0:9001                │
│  • Accepts incoming TCP connections     │
└────────────────┬────────────────────────┘
                 │
                 │ New Connection
                 ▼
┌─────────────────────────────────────────┐
│  Session (Per-Connection Handler)       │
│  • WebSocket Handshake (HTTP→WS)        │
│  • async_read() → Buffer               │
│  • async_write() → Echo back            │
│  • Error handling & cleanup             │
└─────────────────────────────────────────┘
```

**Key Components:**
- `net::io_context`: Event loop (single-threaded)
- `tcp::acceptor`: Listens for connections
- `websocket::stream`: Handles WS protocol
- `Session`: Per-connection state & handlers

## Next Steps

1. **Understand the Flow**: Run the server, test with client, observe logs
2. **Study the Code**: Read `src/quick_start/beast_server.cpp`
3. **Experiment**: Add features (broadcast, rooms, persistence)
4. **Deep Dive**: Implement raw WebSocket from scratch (Phase 2)

## Documentation

See [WEBSOCKET_MASTERY.md](WEBSOCKET_MASTERY.md) for comprehensive guide:
- Protocol specifications (RFC 6455)
- Frame structure & parsing
- System design patterns
- Performance optimization
- Production deployment

## Project Phases

- ✅ **Phase 0**: Quick Start (Boost.Beast) - *Current*
- 🔜 **Phase 1**: Raw Implementation (C++ from scratch)
- 🔜 **Phase 2**: Chat System (Pub/Sub, Redis)
- 🔜 **Phase 3**: Production (Scaling, monitoring)

---

**Author**: Parihar  
**Focus**: System Design & Distributed Systems  
**Goal**: Master WebSocket protocol from ground up
