#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// WebSocket session - handles a single connection
class Session : public std::enable_shared_from_this<Session> {
private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    explicit Session(tcp::socket socket) 
        : ws_(std::move(socket)) {}

    // Start the asynchronous operation
    void run() {
        // Accept the WebSocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &Session::on_accept,
                shared_from_this()
            )
        );
    }

private:
    void on_accept(beast::error_code ec) {
        if (ec) {
            std::cerr << "Accept error: " << ec.message() << std::endl;
            return;
        }

        std::cout << "WebSocket connection accepted" << std::endl;
        
        // Read a message
        do_read();
    }

    void do_read() {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Session::on_read,
                shared_from_this()
            )
        );
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec == websocket::error::closed) {
            std::cout << "Connection closed by client" << std::endl;
            return;
        }

        if (ec) {
            std::cerr << "Read error: " << ec.message() << std::endl;
            return;
        }

        // Echo the message back
        std::cout << "Received: " << beast::make_printable(buffer_.data()) << std::endl;
        
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &Session::on_write,
                shared_from_this()
            )
        );
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            std::cerr << "Write error: " << ec.message() << std::endl;
            return;
        }

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Read another message
        do_read();
    }
};

// Listener - accepts incoming connections
class Listener : public std::enable_shared_from_this<Listener> {
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(ioc) {
        
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Open error: " << ec.message() << std::endl;
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Set option error: " << ec.message() << std::endl;
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            return;
        }

        // Start listening for connections
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << std::endl;
            return;
        }

        std::cout << "WebSocket server listening on " 
                  << endpoint.address() << ":" << endpoint.port() << std::endl;
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            beast::bind_front_handler(
                &Listener::on_accept,
                shared_from_this()
            )
        );
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        } else {
            // Create the session and run it
            std::make_shared<Session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

int main(int argc, char* argv[]) {
    // Default configuration
    const char* host = "0.0.0.0";
    unsigned short port = 9001;

    // Parse command line arguments
    if (argc >= 2) {
        port = static_cast<unsigned short>(std::atoi(argv[1]));
    }

    std::cout << "=== WebSocket Echo Server (Boost.Beast) ===" << std::endl;
    std::cout << "Quick Start Implementation" << std::endl;
    std::cout << "===========================================" << std::endl;

    try {
        // The io_context is required for all I/O
        net::io_context ioc{1}; // Single thread

        // Create and launch the listener
        std::make_shared<Listener>(
            ioc,
            tcp::endpoint{net::ip::make_address(host), port}
        )->run();

        // Run the I/O service
        ioc.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
