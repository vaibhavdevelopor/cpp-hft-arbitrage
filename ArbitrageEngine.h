#pragma once
#include "MarketData.h"
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

struct ArbitrageResult {
    double buy_price;
    double sell_price;
    double gross_profit;
    double total_fees;
    double net_profit;
};

class ArbitrageEngine {
public:
    ArbitrageEngine(MarketData& data, std::atomic<bool>& running);
    ~ArbitrageEngine();

    void start();

    // Static method so it can be easily tested without needing a full engine
    static ArbitrageResult calculateArbitrage(double binance_bid, double binance_ask, double coinbase_bid, double coinbase_ask);

private:
    MarketData& marketData;
    std::ofstream logFile;
    
    // Async Logging infrastructure
    std::vector<std::string> logQueue;
    std::mutex logMutex;
    std::thread loggerThread;
    std::atomic<bool>& isRunning;
    
    void loggerLoop();
    std::string getTimeStr();
};
