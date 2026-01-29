# High-Frequency Crypto Arbitrage Engine (C++)

![C++](https://img.shields.io/badge/C++-17-blue.svg) ![Latency](https://img.shields.io/badge/Latency-%3C2%C2%B5s-brightgreen) ![License](https://img.shields.io/badge/License-MIT-green)

A high-performance, multi-threaded trading engine designed to detect arbitrage opportunities between Binance and Coinbase with sub-microsecond internal latency.

## 🚀 Key Performance Metrics
* **Internal Latency:** < 5 microseconds (Benchmarked at ~2µs on consumer hardware).
* **Throughput:** Capable of processing 500+ market ticks/second without thread blocking.
* **Architecture:** Lock-free event loop using `std::atomic` for inter-thread communication.

## 🛠 Tech Stack
* **Core:** C++17 (MSVC Optimized)
* **Networking:** Boost.Asio & Boost.Beast (Asynchronous WebSockets)
* **Parsing:** Boost.JSON (SIMD-optimized parsing)
* **Security:** OpenSSL (TLS 1.2/1.3 Secure Streams)

## ⚡ Architecture Overview
The system utilizes a **Producer-Consumer** model to minimize the "Tick-to-Trade" path:
1.  **Market Data Threads:** Two parallel threads maintain persistent WebSocket connections to Binance and Coinbase.
2.  **Zero-Copy Logic:** Price updates are written to `std::atomic<double>` memory blocks to avoid mutex locking overhead.
3.  **Signal Engine:** A dedicated analysis thread polls atomic state vectors and executes spreads in O(1) time.

## 📈 Live Benchmark
(See screenshot below for real-time latency measurements)
<img width="1919" height="635" alt="image" src="https://github.com/user-attachments/assets/dc3ce591-eb33-4881-b3f6-bfd9d3989fb4" />
