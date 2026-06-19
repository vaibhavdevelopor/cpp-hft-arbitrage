#include "ExchangeClient.h"
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
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

void BinanceClient::run() {
    auto const host = "stream.binance.com";
    auto const port = "9443";
    auto const target = "/ws/btcusdt@bookTicker";

    while (true) {
        try {
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
            
            ws.set_option(websocket::stream_base::decorator(
                [](websocket::request_type& req) {
                    req.set(http::field::user_agent, "crypto-client-v1");
                }));

            ws.handshake(host, target);

            beast::flat_buffer buffer;
            while(true) {
                ws.read(buffer);
                std::string received_data = beast::buffers_to_string(buffer.data());
                
                if (ws.got_text()) {
                     try {
                        json::value jv = json::parse(received_data);
                        if(jv.as_object().contains("b") && jv.as_object().contains("a")) {
                             std::string bid_str = json::value_to<std::string>(jv.as_object().at("b"));
                             std::string ask_str = json::value_to<std::string>(jv.as_object().at("a"));
                             
                             // Bug 4 Fix: Parse BEFORE locking to prevent torn state
                             double bid_val = std::stod(bid_str);
                             double ask_val = std::stod(ask_str);
                             
                             std::lock_guard<std::mutex> lock(marketData.dataMutex);
                             marketData.binance_state.bid = bid_val;
                             marketData.binance_state.ask = ask_val;
                             marketData.update_counter++;
                        }
                     } catch(...) {}
                }
                buffer.consume(buffer.size());
            }
        } catch(std::exception const& e) {
            std::cerr << "Binance Fail: " << e.what() << " - Reconnecting in 3s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

void CoinbaseClient::run() {
    auto const host = "ws-feed.exchange.coinbase.com";
    auto const port = "443";
    auto const target = "/";

    while (true) {
        try {
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

            ws.set_option(websocket::stream_base::decorator(
                [](websocket::request_type& req) {
                    req.set(http::field::user_agent, "crypto-client-v1");
                }));

            ws.handshake(host, target);

            std::string subscribe_msg = R"({"type": "subscribe", "channels": [{"name": "ticker", "product_ids": ["BTC-USD"]}]})";
            ws.write(net::buffer(subscribe_msg));

            beast::flat_buffer buffer;
            while(true) {
                ws.read(buffer);
                std::string received_data = beast::buffers_to_string(buffer.data());
                
                try {
                    json::value jv = json::parse(received_data);
                    if (jv.as_object().contains("best_bid") && jv.as_object().contains("best_ask")) {
                        std::string bid_str = json::value_to<std::string>(jv.as_object().at("best_bid"));
                        std::string ask_str = json::value_to<std::string>(jv.as_object().at("best_ask"));
                        
                        // Bug 4 Fix: Parse BEFORE locking to prevent torn state
                        double bid_val = std::stod(bid_str);
                        double ask_val = std::stod(ask_str);
                        
                        std::lock_guard<std::mutex> lock(marketData.dataMutex);
                        marketData.coinbase_state.bid = bid_val;
                        marketData.coinbase_state.ask = ask_val;
                        marketData.update_counter++;
                    }
                } catch(...) {}

                buffer.consume(buffer.size());
            }
        } catch(std::exception const& e) {
            std::cerr << "Coinbase Fail: " << e.what() << " - Reconnecting in 3s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}
