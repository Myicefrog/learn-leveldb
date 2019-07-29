#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

namespace leveldb {

enum ValueType { kTypeDeletion = 0x0, kTypeValue = 0x1 };

}

#endif
