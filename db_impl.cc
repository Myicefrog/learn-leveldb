#include "db_impl.h"
#include "dbformat.h"
#include "coding.h"

#include "write_batch_internal.h"
#include "memtable.h"

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
    : dbname_(dbname),
    internal_comparator_(raw_options.comparator),
    last_sequence(0),
    mem_(nullptr)
{}

DBImpl::~DBImpl()
{
  if (mem_ != nullptr) mem_->Unref();
}


Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& val) {
  return DB::Put(o, key, val);
}

DB::~DB() = default;

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
  
  *dbptr = nullptr;
  DBImpl* impl = new DBImpl(options, dbname);
  *dbptr = impl;

  WritableFile* lfile;
  Status s = options.env->NewWritableFile(dbname,&lfile);
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

  Status status;
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


}
