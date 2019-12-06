#ifndef STORAGE_LEVELDB_DB_TABLE_CACHE_H_
#define STORAGE_LEVELDB_DB_TABLE_CACHE_H_

#include <stdint.h>

#include <string>

#include "port.h"
#include "options.h"

namespace leveldb {
class Env;

class TableCache {
 public:
  TableCache(const std::string& dbname, const Options& options, int entries){};
  ~TableCache(){};
};
}

#endif  // STORAGE_LEVELDB_DB_TABLE_CACHE_H_