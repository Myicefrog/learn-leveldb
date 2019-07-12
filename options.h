#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>

namespace leveldb {

struct Options {
};

struct WriteOptions {
	WriteOptions() = default;
	bool sync = false;
};

}

#endif
