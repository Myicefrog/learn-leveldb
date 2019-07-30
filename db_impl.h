#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <atomic>
#include <deque>
#include <set>
#include <string>

#include "db.h"
#include "log_writer.h"

namespace leveldb {

class DBImpl : public DB {

 public:
  DBImpl(const Options& options, const std::string& dbname);

  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;

  ~DBImpl() override;

  // Implementations of the DB interface
  Status Put(const WriteOptions&, const Slice& key,
             const Slice& value) override;
  
  Status Write(const std::string rep_) override;

 private:
  friend class DB;
  const std::string dbname_;
  log::Writer* log_;

};

}

#endif
