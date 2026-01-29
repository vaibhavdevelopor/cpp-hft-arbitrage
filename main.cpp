#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <iomanip> // For std::fixed
#include <openssl/ssl.h>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

std::atomic<double> binance_price{0.0};
std::atomic<double> coinbase_price{0.0};

// --- Helper: Get Current Time String ---
std::string get_time() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void run_binance() {
    try {
        auto const host = "stream.binance.com";
        auto const port = "9443";
        auto const target = "/ws/btcusdt@trade";

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        
        tcp::resolver resolver{ioc};
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws{ioc, ctx};
        
        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));
        beast::get_lowest_layer(ws).connect(results);
        SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host);
        ws.next_layer().handshake(ssl::stream_base::client);
        beast::get_lowest_layer(ws).expires_never();
        
        ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        
        // Binance User Agent
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "crypto-client-v1");
            }));

        ws.handshake(host, target);

        beast::flat_buffer buffer;
        while(true) {
            ws.read(buffer);
            std::string received_data = beast::buffers_to_string(buffer.data());
            
            // Handle Ping/Pong (Keep connection alive)
            if (ws.got_text()) {
                 try {
                    json::value jv = json::parse(received_data);
                    // Binance sends price in "p"
                    if(jv.as_object().contains("p")) {
                         std::string price_str = json::value_to<std::string>(jv.as_object().at("p"));
                         binance_price = std::stod(price_str);
                    }
                 } catch(...) {}
            }
            buffer.consume(buffer.size());
        }
    } catch(std::exception const& e) {
        std::cerr << "Binance Fail: " << e.what() << std::endl;
    }
}

void run_coinbase() {
    try {
        // UPDATED HOST: Using the modern Exchange feed
        auto const host = "ws-feed.exchange.coinbase.com";
        auto const port = "443";
        auto const target = "/";

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        tcp::resolver resolver{ioc};
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws{ioc, ctx};

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));
        beast::get_lowest_layer(ws).connect(results);
        SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host);
        ws.next_layer().handshake(ssl::stream_base::client);
        beast::get_lowest_layer(ws).expires_never();
        
        ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

        // --- FIX: Add User-Agent Header (Crucial for Coinbase) ---
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "crypto-client-v1");
            }));

        ws.handshake(host, target);

        // Subscribe to BTC-USD ticker
        std::string subscribe_msg = R"({"type": "subscribe", "channels": [{"name": "ticker", "product_ids": ["BTC-USD"]}]})";
        ws.write(net::buffer(subscribe_msg));

        beast::flat_buffer buffer;
        while(true) {
            ws.read(buffer);
            std::string received_data = beast::buffers_to_string(buffer.data());
            
            try {
                json::value jv = json::parse(received_data);
                if (jv.as_object().contains("price")) {
                    std::string price_str = json::value_to<std::string>(jv.as_object().at("price"));
                    coinbase_price = std::stod(price_str);
                }
            } catch(...) {}

            buffer.consume(buffer.size());
        }
    } catch(std::exception const& e) {
        std::cerr << "Coinbase Fail: " << e.what() << std::endl;
    }
}

int main() {
    // Launch threads
    std::thread t1(run_binance);
    std::thread t2(run_coinbase);

    std::cout << "Engine Started. Waiting for data streams..." << std::endl;
    double total_profit = 0.0;
    int trade_count = 0;

   // Monitor Loop
    while(true) {
        // 1. Start timer
        auto start_time = std::chrono::high_resolution_clock::now();

        double b = binance_price;
        double c = coinbase_price;

        if (b > 0 && c > 0) {
            double diff = b - c;
            
            // 2. Stop timer & calculate latency
            auto end_time = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

            // --- ANSI COLORS ---
            const std::string GREEN  = "\033[1;32m";
            const std::string RED    = "\033[1;31m";
            const std::string RESET  = "\033[0m";
            const std::string GRAY   = "\033[90m";
            const std::string CYAN   = "\033[1;36m";
            const std::string YELLOW = "\033[1;33m";

            // Logic: Green if profitable, Red if negative
            std::string spread_color = (diff > 0) ? GREEN : RED;
            std::string latency_color = (latency < 10) ? GREEN : RED;

            // 3. Print the Dashboard Line
            std::cout << GRAY << "[" << get_time() << "] " << RESET
                      << "Bin: " << std::fixed << std::setprecision(2) << b 
                      << " | Coin: " << c 
                      << " | Spread: " << spread_color << (diff > 0 ? "+" : "") << diff << RESET
                      << " | Latency: " << latency_color << latency << "us" << RESET
                      << std::endl;
            
            // 4. MOCK EXECUTION (The "Money" Logic)
            if(std::abs(diff) > 20.0) {
                 total_profit += std::abs(diff); 
                 trade_count++;
                 
                 std::cout << "\n" << CYAN << "   >>> 💰 ARBITRAGE SIGNAL DETECTED <<<" << RESET << std::endl;
                 std::cout << YELLOW << "   [MOCK TRADE] Executing Buy/Sell Order..." << RESET << std::endl;
                 std::cout << GREEN  << "   PROFIT: +$" << std::abs(diff) << RESET << std::endl;
                 std::cout << GRAY   << "   Total Session Profit: $" << total_profit 
                           << " (" << trade_count << " trades)" << RESET << "\n" << std::endl;

                 // Sleep to simulate API execution time & prevent spam
                 std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        // Sleep to save CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    t1.join();
    t2.join();
    return 0;
}