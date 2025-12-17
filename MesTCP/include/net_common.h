#ifndef NET_COMMON
#define NET_COMMON

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <string>
#include <iostream>
#include <chrono>
#include <limits>
#include <array>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <D:/Boost/include/boost-1_90/boost/asio.hpp>

#endif