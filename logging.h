#ifndef STORAGE_LEVELDB_UTIL_LOGGING_H_
#define STORAGE_LEVELDB_UTIL_LOGGING_H_

#include <stdint.h>
#include <stdio.h>

#include <string>

#include "port.h"

namespace leveldb {

class Slice;

void AppendEscapedStringTo(std::string* str, const Slice& value);

std::string EscapeString(const Slice& value);


}


#endif 
