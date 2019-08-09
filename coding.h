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

void PutFixed64(std::string* dst, uint64_t value);
char* EncodeVarint64(char* dst, uint64_t value);

bool GetLengthPrefixedSlice(Slice* input, Slice* result);

bool GetVarint32(Slice* input, uint32_t* value);

inline void EncodeFixed64(char* dst, uint64_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / str (ARM) instruction.
    std::memcpy(buffer, &value, sizeof(uint64_t));
    return;
  }

  // Platform-independent code.
  // Currently, only gcc optimizes this to a single mov / str instruction.
  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
  buffer[4] = static_cast<uint8_t>(value >> 32);
  buffer[5] = static_cast<uint8_t>(value >> 40);
  buffer[6] = static_cast<uint8_t>(value >> 48);
  buffer[7] = static_cast<uint8_t>(value >> 56);
}

inline uint64_t DecodeFixed64(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / ldr (ARM) instruction.
    uint64_t result;
    std::memcpy(&result, buffer, sizeof(uint64_t));
    return result;
  }

  // Platform-independent code.
  // Clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint64_t>(buffer[0])) |
         (static_cast<uint64_t>(buffer[1]) << 8) |
         (static_cast<uint64_t>(buffer[2]) << 16) |
         (static_cast<uint64_t>(buffer[3]) << 24) |
         (static_cast<uint64_t>(buffer[4]) << 32) |
         (static_cast<uint64_t>(buffer[5]) << 40) |
         (static_cast<uint64_t>(buffer[6]) << 48) |
         (static_cast<uint64_t>(buffer[7]) << 56);
}

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  if (port::kLittleEndian) {
	  std::cout<<"kLittleEndian"<<std::endl;
    uint32_t result;
    std::memcpy(&result, buffer, sizeof(uint32_t));
    return result;
  }

  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

inline void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  if (port::kLittleEndian) {
    std::memcpy(buffer, &value, sizeof(uint32_t));
    return;
  }

  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
}

const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value);

inline const char* GetVarint32Ptr(const char* p, const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const uint8_t*>(p));
    if ((result & 128) == 0) {
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}

}

#endif
