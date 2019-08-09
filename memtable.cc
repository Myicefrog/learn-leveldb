#include "memtable.h"
#include "dbformat.h"
#include "comparator.h"
#include "env.h"
#include "coding.h"

namespace leveldb {

MemTable::MemTable(const InternalKeyComparator& comparator)
    : comparator_(comparator), refs_(0) {}

MemTable::~MemTable() { assert(refs_ == 0); }

void MemTable::Add(SequenceNumber s, ValueType type, const Slice& key,
                   const Slice& value) {

}

}
