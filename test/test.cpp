// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\leveldb\leveldb\db.h"

#if defined _DEBUG

#if defined LEVELDB_DLL
#pragma comment(lib,"..\\DebugDll\\leveldb_d.lib")
#else
#pragma comment(lib,"..\\Debug\\leveldb_d.lib")
#endif

#else

#if defined LEVELDB_DLL
#pragma comment(lib,"..\\ReleaseDll\\leveldb.lib")
#else
#pragma comment(lib,"..\\Release\\leveldb.lib")
#endif

#endif

#include <conio.h>

#define DB_TEST

#if defined LEVELDB_DLL
#undef DB_BENCH
#undef DB_TEST
#endif
#if defined DB_BENCH
#include "..\leveldb\db\db_bench.cc"
#elif defined DB_TEST
#include "..\leveldb\db\db_test.cc"
#else 
int _tmain(int argc, _TCHAR* argv[])
{
    leveldb::DB* db = NULL;
    leveldb::Options options;
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
    //if (s.ok()) s = db->Delete(leveldb::WriteOptions(), key1);
    delete db;
    //s =  leveldb::DestroyDB("c:/tmp/testdb",options);
    if(!s.ok()){
        printf("%s",s.ToString().c_str());
    }
	return 0;
}
#endif

