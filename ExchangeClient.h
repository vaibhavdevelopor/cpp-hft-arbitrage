#pragma once
#include "MarketData.h"

class ExchangeClient {
public:
    ExchangeClient(MarketData& data) : marketData(data) {}
    virtual ~ExchangeClient() = default;

    virtual void run() = 0;

protected:
    MarketData& marketData;
};

class BinanceClient : public ExchangeClient {
public:
    BinanceClient(MarketData& data) : ExchangeClient(data) {}
    void run() override;
};

class CoinbaseClient : public ExchangeClient {
public:
    CoinbaseClient(MarketData& data) : ExchangeClient(data) {}
    void run() override;
};
