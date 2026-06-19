#include "ArbitrageEngine.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <filesystem>

ArbitrageEngine::ArbitrageEngine(MarketData& data, std::atomic<bool>& running) 
    : marketData(data), isRunning(running) {
    // Bug 2 Fix: Only write CSV header if the file is new or empty
    bool needsHeader = !std::filesystem::exists("market_data.csv") ||
                        std::filesystem::file_size("market_data.csv") == 0;
    
    logFile.open("market_data.csv", std::ios::app);
    if(logFile.is_open() && needsHeader) {
        logFile << "Timestamp,BinanceAsk,CoinbaseBid,MaxGrossSpread,NetProfit,LatencyUs\n";
        logFile.flush();
    }
    loggerThread = std::thread(&ArbitrageEngine::loggerLoop, this);
}

ArbitrageEngine::~ArbitrageEngine() {
    // Bug 3 Fix: Wait for logger thread to finish (it does its own final flush)
    if(loggerThread.joinable()) {
        loggerThread.join();
    }
    
    // Safety flush: at this point, start() has returned and loggerLoop has returned,
    // so no other thread is touching logQueue. No lock needed.
    if (!logQueue.empty() && logFile.is_open()) {
        for (const auto& entry : logQueue) {
            logFile << entry;
        }
        logFile.flush();
    }
    
    if(logFile.is_open()) {
        logFile.close();
    }
}

void ArbitrageEngine::loggerLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        std::vector<std::string> localQueue;
        {
            std::lock_guard<std::mutex> lock(logMutex);
            localQueue.swap(logQueue);
        }
        
        if (!localQueue.empty() && logFile.is_open()) {
            for (const auto& entry : localQueue) {
                logFile << entry;
            }
            logFile.flush();
        }
    }
    
    // Bug 3 Fix: Final flush after shutdown — catch data pushed while we were sleeping
    std::vector<std::string> finalQueue;
    {
        std::lock_guard<std::mutex> lock(logMutex);
        finalQueue.swap(logQueue);
    }
    if (!finalQueue.empty() && logFile.is_open()) {
        for (const auto& entry : finalQueue) {
            logFile << entry;
        }
        logFile.flush();
    }
}

ArbitrageResult ArbitrageEngine::calculateArbitrage(double binance_bid, double binance_ask, double coinbase_bid, double coinbase_ask) {
    ArbitrageResult result;
    const double FEE_RATE = 0.001; // 0.1% trading fee per exchange

    // Option 1: Buy on Binance, Sell on Coinbase
    double buy_binance_cost = binance_ask;
    double sell_coinbase_rev = coinbase_bid;
    double gross1 = sell_coinbase_rev - buy_binance_cost;
    double fees1 = (buy_binance_cost * FEE_RATE) + (sell_coinbase_rev * FEE_RATE);
    double net1 = gross1 - fees1;

    // Option 2: Buy on Coinbase, Sell on Binance
    double buy_coinbase_cost = coinbase_ask;
    double sell_binance_rev = binance_bid;
    double gross2 = sell_binance_rev - buy_coinbase_cost;
    double fees2 = (buy_coinbase_cost * FEE_RATE) + (sell_binance_rev * FEE_RATE);
    double net2 = gross2 - fees2;

    if (net1 > net2) {
        result.buy_price = buy_binance_cost;
        result.sell_price = sell_coinbase_rev;
        result.gross_profit = gross1;
        result.total_fees = fees1;
        result.net_profit = net1;
    } else {
        result.buy_price = buy_coinbase_cost;
        result.sell_price = sell_binance_rev;
        result.gross_profit = gross2;
        result.total_fees = fees2;
        result.net_profit = net2;
    }

    return result;
}

std::string ArbitrageEngine::getTimeStr() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void ArbitrageEngine::start() {
    std::cout << "Engine Started. Waiting for data streams..." << std::endl;

    double totalProfit = 0.0;
    int tradeCount = 0;
    uint64_t last_update = 0;

    while(isRunning) {
        // Spin-wait lock-free until new data arrives
        uint64_t current_update = marketData.update_counter.load();
        if (current_update == last_update) {
            std::this_thread::yield();
            continue;
        }
        last_update = current_update;

        auto start_time = std::chrono::high_resolution_clock::now();

        double bb, ba, cb, ca;
        {
            // Extremely short lock to copy variables
            std::lock_guard<std::mutex> lock(marketData.dataMutex);
            bb = marketData.binance_state.bid;
            ba = marketData.binance_state.ask;
            cb = marketData.coinbase_state.bid;
            ca = marketData.coinbase_state.ask;
        }

        if (bb > 0 && ba > 0 && cb > 0 && ca > 0) {

            // --- Fee Calculation Logic using our testable function ---
            ArbitrageResult result = calculateArbitrage(bb, ba, cb, ca);

            // Bug 5 Fix: Measure latency AFTER computation, not before
            auto end_time = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

            const std::string GREEN  = "\033[1;32m";
            const std::string RED    = "\033[1;31m";
            const std::string RESET  = "\033[0m";
            const std::string GRAY   = "\033[90m";
            const std::string CYAN   = "\033[1;36m";
            const std::string YELLOW = "\033[1;33m";

            std::string spread_color = (result.net_profit > 0) ? GREEN : RED;
            std::string latency_color = (latency < 10) ? GREEN : RED;

            std::cout << GRAY << "[" << getTimeStr() << "] " << RESET
                      << "Bin Ask: " << std::fixed << std::setprecision(2) << ba 
                      << " | Coin Bid: " << cb 
                      << " | Max Gross Spread: " << result.gross_profit
                      << " | Net: " << spread_color << (result.net_profit > 0 ? "+" : "") << result.net_profit << RESET
                      << " | Latency: " << latency_color << latency << "us" << RESET
                      << std::endl;
            
            // Push to async logger
            std::string logLine = getTimeStr() + "," + 
                                  std::to_string(ba) + "," + 
                                  std::to_string(cb) + "," + 
                                  std::to_string(result.gross_profit) + "," + 
                                  std::to_string(result.net_profit) + "," + 
                                  std::to_string(latency) + "\n";
            {
                std::lock_guard<std::mutex> lock(logMutex);
                logQueue.push_back(logLine);
            }
            
            // Only execute if it's ACTUALLY profitable after fees
            if(result.net_profit > 0.0) {
                 totalProfit += result.net_profit; 
                 tradeCount++;
                 
                 std::cout << "\n" << CYAN << "   >>> 💰 ARBITRAGE SIGNAL DETECTED <<<" << RESET << std::endl;
                 std::cout << YELLOW << "   [MOCK TRADE] Executing Buy @ $" << result.buy_price << " & Sell @ $" << result.sell_price << RESET << std::endl;
                 std::cout << RED    << "   FEES PAID: -$" << result.total_fees << RESET << std::endl;
                 std::cout << GREEN  << "   NET PROFIT: +$" << result.net_profit << RESET << std::endl;
                 std::cout << GRAY   << "   Total Session Profit: $" << totalProfit 
                           << " (" << tradeCount << " trades)" << RESET << "\n" << std::endl;

                 std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
