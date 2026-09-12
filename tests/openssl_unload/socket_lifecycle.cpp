#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>
#include <sstream>
#include <chrono>
#define LOG(level) std::ostringstream()
namespace big { inline std::atomic_bool g_running{true}; }
#include "server/socket_client.hpp"
int main() {
    // Reserve an unused local port without listening; connections fail locally.
    asio::io_context context;
    asio::ip::tcp::acceptor reserved(context);
    reserved.open(asio::ip::tcp::v4());
    reserved.bind({asio::ip::address_v4::loopback(), 0});
    auto endpoint = std::string("127.0.0.1:") + std::to_string(reserved.local_endpoint().port());
    for (bool tls : {false, true}) {
        for (int i = 0; i < 50; ++i) {
            big::socket_client client(endpoint, tls);
            if (i % 2) std::this_thread::sleep_for(std::chrono::milliseconds(2));
            client.disconnect();
            client.disconnect();
            client.reconnect();
        }
    }
    OPENSSL_cleanup();
    std::cout << "PASS: WS/WSS immediate stop, failed connect, reconnect and destruction\n";
}
