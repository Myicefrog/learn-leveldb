#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <stdint.h>
#include <stdio.h>

#include <iostream>

#include "options.h"
#include "slice.h"
#include "status.h"
#include "write_batch.h"

namespace leveldb {
class DB {
  public:
	DB() = default;

    //不允许使用拷贝构造函数
    DB(const DB&) = delete;
	DB& operator=(const DB&) = delete;

	virtual ~DB();

	static Status Open(const Options& options, const std::string& name, DB** dbptr);

	//纯虚函数可以有默认实现
	virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& value) = 0;
    
    virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;

};

}

#endif
