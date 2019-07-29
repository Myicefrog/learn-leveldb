#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>
#include "env.h"

namespace leveldb {

struct Options {
  Options();

  Env* env;

};

struct WriteOptions {
	WriteOptions() = default;
	bool sync = false;
};

}

#endif
