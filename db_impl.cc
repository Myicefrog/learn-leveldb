#include "db_impl.h"
#include "dbformat.h"
#include "coding.h"

namespace leveldb {

DBImpl::DBImpl(const Options& raw_options, const std::string& dbname)
    : dbname_(dbname)
{}

DBImpl::~DBImpl()
{}


Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& val) {
  return DB::Put(o, key, val);
}

DB::~DB() = default;

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
  
  *dbptr = nullptr;
  DBImpl* impl = new DBImpl(options, dbname);
  *dbptr = impl;

  
  Status s;
  return s;
}

Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
  std::cout<<"DB::Put"<<std::endl;

  std::string rep_;
  rep_.push_back(static_cast<char>(kTypeValue)); 
  PutLengthPrefixedSlice(&rep_, key);
  PutLengthPrefixedSlice(&rep_, value);
  
  


  Status s;
  return s;
}

}
