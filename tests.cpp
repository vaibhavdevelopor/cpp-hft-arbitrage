#include <gtest/gtest.h>
#include "ArbitrageEngine.h"

// Test 1: Ensure basic profit math is correct
TEST(ArbitrageEngineTests, FeeCalculationIsCorrect) {
    double binance_bid = 59990.0;
    double binance_ask = 60000.0; // We buy at Ask
    double coinbase_bid = 60100.0; // We sell at Bid
    double coinbase_ask = 60110.0;
    
    ArbitrageResult result = ArbitrageEngine::calculateArbitrage(binance_bid, binance_ask, coinbase_bid, coinbase_ask);
    
    EXPECT_DOUBLE_EQ(result.buy_price, 60000.0);
    EXPECT_DOUBLE_EQ(result.sell_price, 60100.0);
    EXPECT_DOUBLE_EQ(result.gross_profit, 100.0);
    EXPECT_DOUBLE_EQ(result.total_fees, 120.1);
    EXPECT_DOUBLE_EQ(result.net_profit, -20.1);
}

// Test 2: Ensure an identical price results in exactly zero gross profit but negative net profit due to fees
TEST(ArbitrageEngineTests, IdenticalPricesResultInLoss) {
    double price = 50000.0;
    
    ArbitrageResult result = ArbitrageEngine::calculateArbitrage(price, price, price, price);
    
    EXPECT_DOUBLE_EQ(result.gross_profit, 0.0);
    EXPECT_DOUBLE_EQ(result.total_fees, 100.0);
    EXPECT_DOUBLE_EQ(result.net_profit, -100.0);
}

// Test 3: Large spread results in positive net profit
TEST(ArbitrageEngineTests, ProfitableArbitrage) {
    double binance_bid = 59990.0;
    double binance_ask = 60000.0;
    double coinbase_bid = 60500.0;
    double coinbase_ask = 60510.0;
    
    ArbitrageResult result = ArbitrageEngine::calculateArbitrage(binance_bid, binance_ask, coinbase_bid, coinbase_ask);
    
    EXPECT_DOUBLE_EQ(result.gross_profit, 500.0);
    EXPECT_DOUBLE_EQ(result.total_fees, 120.5);
    EXPECT_DOUBLE_EQ(result.net_profit, 379.5);
}
