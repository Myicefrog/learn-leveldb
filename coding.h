#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>

#include "slice.h"
#include "port.h"

namespace leveldb {

void PutLengthPrefixedSlice(std::string* dst, const Slice& value);
void PutVarint32(std::string* dst, uint32_t value);
char* EncodeVarint32(char* dst, uint32_t value);

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  if (port::kLittleEndian) {
	  std::cout<<"kLittleEndian"<<std::endl;
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / ldr (ARM) instruction.
    uint32_t result;
    std::memcpy(&result, buffer, sizeof(uint32_t));
    return result;
  }

  // Platform-independent code.
  // Clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

inline void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  if (port::kLittleEndian) {
	  std::cout<<"port::kLittleEndian is true"<<std::endl;
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / str (ARM) instruction.
    std::memcpy(buffer, &value, sizeof(uint32_t));
    return;
  }

  // Platform-independent code.
  // Currently, only gcc optimizes this to a single mov / str instruction.
  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
}

}

#endif
