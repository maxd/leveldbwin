

#ifndef WIN32EXPORTS_H_
#define WIN32EXPORTS_H_

#if defined LEVELDB_DLL

#if defined DLL_BUILD
#define LEVELDB_EXPORT __declspec(dllexport)
#else
#define LEVELDB_EXPORT __declspec(dllimport)
#endif

#else
#define LEVELDB_EXPORT
#endif

#endif