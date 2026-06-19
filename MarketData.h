#pragma once
#include <atomic>
#include <mutex>

struct ExchangeState {
    double bid{0.0};
    double ask{0.0};
};

// Shared structure to safely store prices across different threads
struct MarketData {
    ExchangeState binance_state;
    ExchangeState coinbase_state;
    
    std::mutex dataMutex;
    std::atomic<uint64_t> update_counter{0};
};
