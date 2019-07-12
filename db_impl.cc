#include "db_impl.h"

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

  string rep_;
  


  Status s;
  return s;
}

}
