#ifndef STORAGE_LEVELDB_INCLUDE_ENV_H_
#define STORAGE_LEVELDB_INCLUDE_ENV_H_

#include "status.h"

namespace leveldb {

class WritableFile;

class Env {

 public:
  Env() = default;

  Env(const Env&) = delete;
  Env& operator=(const Env&) = delete;

  virtual ~Env();

  static Env* Default();

  virtual Status NewWritableFile(const std::string& fname,
                                 WritableFile** result) = 0;

};

class WritableFile {
 public:
  WritableFile() = default;

  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  virtual ~WritableFile();

  virtual Status Append(const Slice& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;
};

}

#endif
