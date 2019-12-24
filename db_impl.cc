#include "db_impl.h"
#include "dbformat.h"
#include "coding.h"

#include "write_batch_internal.h"
#include "memtable.h"
#include "table_cache.h"
#include "version_set.h"
#include "version_edit.h"
#include "filename.h"
#include "iterator.h"
#include "builder.h"

namespace leveldb {

struct DBImpl::Writer {
  explicit Writer(port::Mutex* mu)
      : batch(nullptr), sync(false), done(false), cv(mu) {}

  Status status;
  WriteBatch* batch;
  bool sync;
  bool done;
  port::CondVar cv;
};  

DBImpl::DBImpl(const Options& raw_options, const std::string& dbname)
    : env_(raw_options.env),
    dbname_(dbname),
    table_cache_(new TableCache(dbname_, options_, 990)),
    internal_comparator_(raw_options.comparator),
    last_sequence(0),
    mem_(nullptr),
    imm_(nullptr),
    has_imm_(false),
    background_compaction_scheduled_(false),
    versions_(new VersionSet(dbname_, &options_, table_cache_,&internal_comparator_))
{}

DBImpl::~DBImpl()
{
  delete versions_;
  if (mem_ != nullptr) mem_->Unref();
  if (imm_ != nullptr) imm_->Unref();
}


Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& val) {
  return DB::Put(o, key, val);
}

DB::~DB() = default;

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
  
  *dbptr = nullptr;
  DBImpl* impl = new DBImpl(options, dbname);
  *dbptr = impl;

  uint64_t new_log_number = impl->versions_->NewFileNumber();
  WritableFile* lfile;
  Status s = options.env->NewWritableFile(dbname,&lfile);
  impl->logfile_ = lfile;
  impl->logfile_number_ = new_log_number;
  impl->log_ = new log::Writer(lfile);
  impl->mem_ = new MemTable(impl->internal_comparator_);
  impl->mem_->Ref();

  
  return s;
}

Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
  std::cout<<"DB::Put"<<std::endl;

  WriteBatch batch;
  batch.Put(key, value);
  
  return Write(opt, &batch);
}

Status DBImpl::Write(const WriteOptions& options, WriteBatch* updates)  {

  Writer w(&mutex_);
  w.batch = updates;
  w.sync = options.sync;
  w.done = false;

  Writer* last_writer = &w;
  writers_.push_back(&w);

  Status status = MakeRoomForWrite(updates == nullptr);

  if (updates != nullptr){
    WriteBatch* updates = BuildBatchGroup(&last_writer);
    WriteBatchInternal::SetSequence(updates, last_sequence + 1);
    last_sequence += WriteBatchInternal::Count(updates);
    status = log_->AddRecord(WriteBatchInternal::Contents(updates));
    if(status.ok())
    {
      status = WriteBatchInternal::InsertInto(updates, mem_);
    }
  } 

  return status;

}

Status DBImpl::Get(const ReadOptions& options, const Slice& key,
                   std::string* value) {
  Status s;
  MemTable* mem = mem_;
  SequenceNumber snapshot;
  mem->Ref();

  LookupKey lkey(key, snapshot);
  if (mem->Get(lkey, value, &s)) {
     // Done
  } 
                   
  mem->Unref(); 
  return s;
}

WriteBatch* DBImpl::BuildBatchGroup(Writer** last_writer) {
 // mutex_.AssertHeld();
  assert(!writers_.empty());
  Writer* first = writers_.front();
  WriteBatch* result = first->batch;
  assert(result != nullptr);

  size_t size = WriteBatchInternal::ByteSize(first->batch);

  // Allow the group to grow up to a maximum size, but if the
  // original write is small, limit the growth so we do not slow
  // down the small write too much.
  size_t max_size = 1 << 20;
  if (size <= (128 << 10)) {
    max_size = size + (128 << 10);
  }

  *last_writer = first;
  std::deque<Writer*>::iterator iter = writers_.begin();
  ++iter;  // Advance past "first"
  for (; iter != writers_.end(); ++iter) {
    Writer* w = *iter;
    if (w->sync && !first->sync) {
      // Do not include a sync write into a batch handled by a non-sync write.
      break;
    }

    if (w->batch != nullptr) {
      size += WriteBatchInternal::ByteSize(w->batch);
      if (size > max_size) {
        // Do not make batch too big
        break;
      }

      // Append to *result
      if (result == first->batch) {
        // Switch to temporary batch instead of disturbing caller's batch
        result = tmp_batch_;
        assert(WriteBatchInternal::Count(result) == 0);
        WriteBatchInternal::Append(result, first->batch);
      }
      WriteBatchInternal::Append(result, w->batch);
    }
    *last_writer = w;
  }
  return result;
}

Status DBImpl::MakeRoomForWrite(bool force) {
  Status s;
  while (true) {
    if(!force && (mem_->ApproximateMemoryUsage() <= 4 * 1024 * 1024))
    {
      break;
    }
    else
    {
      uint64_t new_log_number = versions_->NewFileNumber();
      WritableFile* lfile = nullptr;
      s = env_->NewWritableFile(LogFileName(dbname_, new_log_number), &lfile);
      delete log_;
      delete logfile_;
      logfile_ = lfile;
      logfile_number_ = new_log_number;
      log_ = new log::Writer(lfile);
      imm_ = mem_;
      has_imm_.store(true, std::memory_order_release);
      mem_ = new MemTable(internal_comparator_);
      mem_->Ref();
      force = false;
      MaybeScheduleCompaction();
    }
  }  
  return s;
}

void DBImpl::MaybeScheduleCompaction() {
  if (background_compaction_scheduled_) {
    // Already scheduled
  } else {
    background_compaction_scheduled_ = true;
    env_->Schedule(&DBImpl::BGWork, this);
  }
}

void DBImpl::BGWork(void* db) {
  reinterpret_cast<DBImpl*>(db)->BackgroundCall();
}

void DBImpl::BackgroundCall() {
  //MutexLock l(&mutex_);
  BackgroundCompaction();
  background_compaction_scheduled_ = false;
}

void DBImpl::BackgroundCompaction() {
  if (imm_ != nullptr) {
    CompactMemTable();
    return;
  }

}

void DBImpl::CompactMemTable() {
  VersionEdit edit;
  Version* base = versions_->current();
  base->Ref();
  Status s = WriteLevel0Table(imm_, &edit, base);
  base->Unref();

}

Status DBImpl::WriteLevel0Table(MemTable* mem, VersionEdit* edit,Version* base) {
  const uint64_t start_micros = env_->NowMicros();
  FileMetaData meta;
  meta.number = versions_->NewFileNumber();
  pending_outputs_.insert(meta.number);

  Iterator* iter = mem->NewIterator();
  std::cout<<"level-0 table "<<meta.number<<std::endl;


  Status s;
  s = BuildTable(dbname_, env_, options_, table_cache_, iter, &meta);
  return s;
}

}
