// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\leveldb\leveldb\db.h"
#if defined _DEBUG
#pragma comment(lib,"..\\Debug\\leveldb.lib")
#else
#pragma comment(lib,"..\\Release\\leveldb.lib")
#endif

#include <conio.h>

#define DB_BENCH

#if defined DB_BENCH
#include "..\leveldb\db\db_bench.cc"
#elif defined DB_TEST
#include "..\leveldb\db\db_test.cc"
#else 
int _tmain(int argc, _TCHAR* argv[])
{
    leveldb::DB* db;
    leveldb::Options options;
    options.compression = leveldb::kNoCompression;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "c:/tmp/testdb", &db);
    assert(status.ok());

    leveldb::Slice key1 = "one";
    leveldb::Slice key2 = "two";

    leveldb::WriteOptions wo;
    db->Put(wo,key1,"111111111111111111111");

    std::string value;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
    if (s.ok()) s = db->Put(leveldb::WriteOptions(), key2, value);
    if (s.ok()) s = db->Delete(leveldb::WriteOptions(), key1);
    delete db;
    s =  leveldb::DestroyDB("c:/tmp/testdb",options);
    if(!s.ok()){
        printf("%s",s.ToString().c_str());
    }
	return 0;
}
#endif

