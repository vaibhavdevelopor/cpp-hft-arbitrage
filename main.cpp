#include "MarketData.h"
#include "ExchangeClient.h"
#include "ArbitrageEngine.h"
#include <thread>
#include <csignal>
#include <atomic>
#include <iostream>

// Bug 1 Fix: Global shutdown flag controlled by signal handler
std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\n\n[SHUTDOWN] Ctrl+C received. Flushing logs and shutting down cleanly...\n" << std::endl;
    g_running.store(false);
}

int main() {
    // Install signal handler for clean Ctrl+C shutdown
    std::signal(SIGINT, signalHandler);

    // 1. Create the shared Market Data structure
    MarketData marketData;

    // 2. Initialize the Exchange Clients
    BinanceClient binanceClient(marketData);
    CoinbaseClient coinbaseClient(marketData);

    // 3. Initialize the Trading Engine (pass the shared shutdown flag)
    ArbitrageEngine engine(marketData, g_running);

    // 4. Launch exchange listeners in background threads
    //    We detach() instead of join() because:
    //    - These threads are blocked on ws.read() and cannot be interrupted
    //    - Destroying a joinable std::thread calls std::terminate() (instant crash)
    //    - Detaching lets the OS clean them up when the process exits
    std::thread t1([&binanceClient]() { binanceClient.run(); });
    std::thread t2([&coinbaseClient]() { coinbaseClient.run(); });
    t1.detach();
    t2.detach();

    // 5. Run the trading engine in the main thread (blocks until Ctrl+C)
    engine.start();

    // 6. engine goes out of scope here -> destructor runs -> logger flushes -> file closes
    std::cout << "[SHUTDOWN] Engine stopped. All data saved. Goodbye!" << std::endl;
    return 0;
}