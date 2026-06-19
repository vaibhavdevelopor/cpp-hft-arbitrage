cmake_minimum_required(VERSION 3.14)
project(CryptoEngine)

# Use C++ 17
set(CMAKE_CXX_STANDARD 17)

# 1. Find the packages
find_package(Boost REQUIRED COMPONENTS json) 
find_package(OpenSSL REQUIRED)

# 2. Create the executable
add_executable(CryptoEngine main.cpp ExchangeClient.cpp ArbitrageEngine.cpp)

# 3. Link the libraries
target_link_libraries(CryptoEngine PRIVATE Boost::json Boost::headers OpenSSL::SSL OpenSSL::Crypto)

# Specific fix for Windows networking
if(WIN32)
    target_compile_definitions(CryptoEngine PRIVATE _WIN32_WINNT=0x0601)
endif()

# ----------------- UNIT TESTING (Google Test) -----------------
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(CryptoEngineTests tests.cpp ArbitrageEngine.cpp)
target_link_libraries(CryptoEngineTests PRIVATE gtest_main Boost::json Boost::headers OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
    target_compile_definitions(CryptoEngineTests PRIVATE _WIN32_WINNT=0x0601)
endif()

add_test(NAME CryptoEngineTests COMMAND CryptoEngineTests)