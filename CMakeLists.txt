cmake_minimum_required(VERSION 3.10)
project(CryptoEngine)

# Use C++ 17
set(CMAKE_CXX_STANDARD 17)

# 1. Find the packages
# We explicitly ask for the 'json' component now.
find_package(Boost REQUIRED COMPONENTS json) 
find_package(OpenSSL REQUIRED)

# 2. Create the executable
add_executable(CryptoEngine main.cpp)

# 3. Link the libraries
# We link 'Boost::json' specifically (this fixes the Linker error).
# We also link 'Boost::headers' for the rest (Beast/Asio).
target_link_libraries(CryptoEngine PRIVATE Boost::json Boost::headers OpenSSL::SSL OpenSSL::Crypto)

# Specific fix for Windows networking
if(WIN32)
    target_compile_definitions(CryptoEngine PRIVATE _WIN32_WINNT=0x0601)
endif()