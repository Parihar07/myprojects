cmake_minimum_required(VERSION 3.15)
project(WebSocketMastery VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Find Boost (required for Beast) - header-only, no need for components
find_package(Boost 1.70 REQUIRED)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Quick Start: Boost.Beast WebSocket Server
add_executable(websocket_beast_server
    src/quick_start/beast_server.cpp
)
target_link_libraries(websocket_beast_server
    ${Boost_LIBRARIES}
    Threads::Threads
)

# Phase 1: Raw WebSocket Server (from scratch)
# Uncomment when ready to implement
# add_executable(websocket_raw_server
#     src/raw_impl/main.cpp
#     src/raw_impl/event_loop.cpp
#     src/raw_impl/connection.cpp
#     src/raw_impl/frame_parser.cpp
#     src/raw_impl/handshake.cpp
# )
# target_link_libraries(websocket_raw_server Threads::Threads)

# Tests
# add_executable(frame_parser_test tests/frame_parser_test.cpp)
# enable_testing()
# add_test(NAME FrameParserTest COMMAND frame_parser_test)
