#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <atomic>
#include <deque>
#include <set>
#include <string>

#include "db.h"
#include "log_writer.h"
#include "dbformat.h"
#include "env.h"

namespace leveldb {

class MemTable;

class DBImpl : public DB {
 public:
  DBImpl(const Options& options, const std::string& dbname);

  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;

  ~DBImpl() override;

  // Implementations of the DB interface
  Status Put(const WriteOptions&, const Slice& key,
             const Slice& value) override;
  
  Status Write(const WriteOptions& options, WriteBatch* updates) override;

  uint64_t last_sequence;
  MemTable* mem_;

 private:
  friend class DB;
  const std::string dbname_;
  log::Writer* log_;
  struct Writer;
  const InternalKeyComparator internal_comparator_;
  WriteBatch* tmp_batch_;
  port::Mutex mutex_;

  WriteBatch* BuildBatchGroup(Writer** last_writer);
  std::deque<Writer*> writers_;

};

}

#endif
