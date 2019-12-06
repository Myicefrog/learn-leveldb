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
class TableCache;
class VersionSet;
class VersionEdit;
class Version;

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

  Status Get(const ReadOptions& options, const Slice& key,
             std::string* value) override;

  Status WriteLevel0Table(MemTable* mem, VersionEdit* edit, Version* base);
  Status MakeRoomForWrite(bool force);
  void MaybeScheduleCompaction();
  static void BGWork(void* db);
  void BackgroundCall();
  void BackgroundCompaction();
  void CompactMemTable();

  uint64_t last_sequence;
  MemTable* mem_;
  MemTable* imm_;
  std::atomic<bool> has_imm_;

 private:
  friend class DB;
  
  Env* const env_;
  const Options options_;
  const std::string dbname_;
  log::Writer* log_;
  struct Writer;
  const InternalKeyComparator internal_comparator_;
  WriteBatch* tmp_batch_;
  port::Mutex mutex_;
  
  WritableFile* logfile_;
  uint64_t logfile_number_;

  WriteBatch* BuildBatchGroup(Writer** last_writer);
  
  std::deque<Writer*> writers_;
  bool background_compaction_scheduled_;

  TableCache* const table_cache_;
  VersionSet* const versions_;

  std::set<uint64_t> pending_outputs_;

};

}

#endif
