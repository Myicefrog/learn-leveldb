#include <iostream>
#include <string>
#include <cassert>

#include "db_impl.h"

using namespace std;
using namespace leveldb;

int main()
{
    cout<<"hello leveldb"<<endl;

    leveldb::DB* db;

    leveldb::Slice key = "hello";
    leveldb::Slice value = "world";

    leveldb::Options options;

    Status status = DB::Open(options, "./testdb", &db);
	status = db->Put(leveldb::WriteOptions(), key, value);
	
	delete db;

    return 0;
}
