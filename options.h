#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>
#include "env.h"

namespace leveldb {

class Comparator;

struct Options {
  Options();

  Env* env;
  const Comparator* comparator;

};

struct WriteOptions {
	WriteOptions() = default;
	bool sync = false;
};

struct  ReadOptions {
  ReadOptions() = default;

  // If true, all data read from underlying storage will be
  // verified against corresponding checksums.
  bool verify_checksums = false;

  // Should the data read for this iteration be cached in memory?
  // Callers may wish to set this field to false for bulk scans.
  bool fill_cache = true;

  // If "snapshot" is non-null, read as of the supplied snapshot
  // (which must belong to the DB that is being read and which must
  // not have been released).  If "snapshot" is null, use an implicit
  // snapshot of the state at the beginning of this read operation.
  
  //const Snapshot* snapshot = nullptr;
};

}

#endif
