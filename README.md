# High-Frequency Crypto Arbitrage Engine (C++) 🚀

![C++](https://img.shields.io/badge/C++-17-blue.svg) ![Latency](https://img.shields.io/badge/Latency-%3C2%C2%B5s-brightgreen) ![License](https://img.shields.io/badge/License-MIT-green)

An ultra-low latency, multi-threaded High-Frequency Trading (HFT) Arbitrage simulator built in modern C++17.

This engine connects to live WebSocket streams from Binance and Coinbase to monitor the `BTC-USD` order books. It computes cross-exchange arbitrage opportunities in real-time, accurately factoring in a 0.1% transaction fee on both legs to calculate the true Net Profit.

## 🚀 Key Performance Metrics
* **Internal Latency:** < 5 microseconds (Benchmarked at ~2µs on consumer hardware).
* **Throughput:** Capable of processing 500+ market ticks/second without thread blocking.
* **Architecture:** Lock-free event loop using `std::atomic` and `std::mutex` for safe inter-thread communication.

## 🛠 Tech Stack
* **Core:** C++17 (MSVC Optimized)
* **Networking:** Boost.Asio & Boost.Beast (Asynchronous WebSockets)
* **Parsing:** Boost.JSON
* **Security:** OpenSSL (TLS 1.2/1.3 Secure Streams)
* **Testing:** Google Test (gtest)

## ⚡ Architecture Overview
The system utilizes a **Producer-Consumer** model to minimize the "Tick-to-Trade" path:
1. **Market Data Threads:** Two parallel threads maintain persistent WebSocket connections to Binance and Coinbase with auto-reconnect logic.
2. **Lock-Free Spin Waiting:** Price updates are synchronized, and the engine uses an `std::atomic<uint64_t>` counter to instantly detect updates without Mutex starvation.
3. **Double-Buffered Async Logging:** Writes market data and trade signals to a CSV file in a background thread using `std::swap()`, completely removing disk I/O bottlenecks from the critical trading path.
4. **Signal Engine:** A dedicated analysis thread polls atomic state vectors and executes spreads in O(1) time.

## 📈 Live Benchmark
<img width="620" height="356" alt="image" src="https://github.com/user-attachments/assets/fcc5c49b-9687-45ef-917e-6f29689381de" />


## Recent Bug Fixes & Hardening

The engine has recently undergone a full line-by-line audit and the following production-critical issues have been resolved:
- **Graceful Shutdown:** Ctrl+C now triggers a clean shutdown sequence, ensuring all threads exit safely and the final logs are flushed to disk (preventing data corruption or `std::terminate()` crashes).
- **Torn State Prevention:** String-to-double parsing is performed *before* acquiring the `std::mutex`, guaranteeing that malformed JSON payloads will not leave the internal state partially updated.
- **Accurate Profiling:** The latency timer measures the complete tick-to-decision pipeline.
- **Data Integrity:** The CSV logger verifies file existence to prevent duplicate headers on restarts.

## Prerequisites

- **CMake** (v3.14+)
- **Boost** (components: `json`, `headers`)
- **OpenSSL**
- A C++17 compatible compiler (MSVC, GCC, Clang)

*(Note: `gtest` is fetched automatically via CMake FetchContent)*

## Build Instructions

```bash
# Generate the build files
cmake -B build

# Compile the project
cmake --build build
```

## Running the Engine

To start the bot and connect to the live exchange feeds:
```bash
./build/Debug/CryptoEngine.exe
```

To run the unit tests:
```bash
./build/Debug/CryptoEngineTests.exe
```

## Logs & Output

The bot prints live data to the terminal and simultaneously logs tick data to `market_data.csv`. The log includes:
- Timestamp
- Binance Ask Price (Buy leg)
- Coinbase Bid Price (Sell leg)
- Max Gross Spread
- Net Profit (after 0.1% fees)
- Pipeline Latency (in microseconds)
