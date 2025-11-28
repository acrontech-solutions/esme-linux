#ifndef TCPBRIDGE_H
#define TCPBRIDGE_H

#include <vector>
#include <cstdint>
#include "json.hpp"

using json = nlohmann::json;
 

constexpr uint8_t SLIP_END = 0xc0;
constexpr uint8_t SLIP_ESC = 0xdb;
constexpr uint8_t SLIP_ESC_END = 0xdc;
constexpr uint8_t SLIP_ESC_ESC = 0xdd;

std::vector<uint8_t> slipEncoding(std::vector<uint8_t> data);
json processPackage(std::vector<uint8_t> data);

#endif