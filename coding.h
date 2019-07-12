#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>

#include "slice.h"

namespace leveldb {

void PutVarint32(std::string* dst, uint32_t value);

}

#endif
