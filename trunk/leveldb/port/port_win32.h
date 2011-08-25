

#ifndef STORAGE_LEVELDB_PORT_PORT_WIN32_H_
#define STORAGE_LEVELDB_PORT_PORT_WIN32_H_

#ifndef OS_WIN
#define OS_WIN
#endif

#if defined _MSC_VER
#define COMPILER_MSVC
#endif


#if _MSC_VER >= 1600
#include <cstdint>
#else
#include <stdint.h>
#endif

#include <string>
#include <cstring>
#include <list>
#include "../port/atomic_pointer.h"



typedef INT64 int64;

#ifdef min
#undef min
#endif

#ifdef small
#undef small
#endif

#define snprintf _snprintf
#define va_copy(a, b) do { (a) = (b); } while (0)

# if !defined(DISALLOW_COPY_AND_ASSIGN)
#  define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)
#endif 

#if defined _WIN32_WINNT_VISTA 
#define USE_VISTA_API
#endif

#pragma warning(disable:4996)
#pragma warning(disable:4018)
#pragma warning(disable:4355)
#pragma warning(disable:4244)
#pragma warning(disable:4800)
//#pragma warning(disable:4996)
    
namespace leveldb
{
namespace port
{
#if defined _M_IX86
    static const bool kLittleEndian = true;
#endif

class Event
{
public:
    Event(bool bSignal = true,bool ManualReset = false);
    ~Event();
    void Wait(DWORD Milliseconds = INFINITE);
    void Signal();
    void UnSignal();
private:
    HANDLE _hEvent;
};

class Mutex 
{
public:
    friend class CondVarNew;
    Mutex();
    ~Mutex();
    void Lock();
    void Unlock();
    BOOL TryLock();
    void AssertHeld();

private:
    CRITICAL_SECTION _cs;
    DISALLOW_COPY_AND_ASSIGN(Mutex);
};

#if defined USE_VISTA_API
class RWLock
{
public:
    RWLock();
    ~RWLock();
    void ReadLock();
    void ReadUnlock();
    void WriteLock();
    void WriteUnlock();
private:
    SRWLOCK _srw;
    DISALLOW_COPY_AND_ASSIGN(RWLock);
};
#else
#define RWLock Mutex
#define ReadLock Lock
#define ReadUnlock Unlock
#define WriteLock Lock
#define WriteUnlock Unlock
#endif

class AutoLock
{
public:
    explicit AutoLock(Mutex& mu) : _mu(mu)
    {
        _mu.Lock();
    }
    ~AutoLock()
    {
        _mu.Unlock();
    }
private:
    Mutex& _mu;
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

class AutoUnlock
{
public:
    explicit AutoUnlock(Mutex& mu) : _mu(mu)
    {
        _mu.Unlock();
    }
    ~AutoUnlock()
    {
        _mu.Lock();
    }
private:
    Mutex& _mu;
    DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

class CondVarOld
{
public:
    typedef std::list<HANDLE>::iterator HandleIter;
    explicit CondVarOld(Mutex* mu);
    ~CondVarOld();
    void Wait();
    void timedWait(DWORD dwMilliseconds);
    void Signal();
    void SignalAll();

private:
    Mutex* _user_lock;
    HANDLE GetWaitingHandle();
    void CloseRecyclingHandles();
    Mutex _internal_lock;
    std::list<HANDLE> _waiting_handles;
    std::list<HANDLE> _recycling_handles;
    size_t _alloced_handles_count;
    enum RunState { SHUTDOWN = 0, RUNNING = 64213 } _run_state;
    DISALLOW_COPY_AND_ASSIGN(CondVarOld);
};

class CondVarNew
{
public:
    explicit CondVarNew(Mutex* mu);
    ~CondVarNew();
    void Wait();
    void Signal();
    void SignalAll();
private:
    CONDITION_VARIABLE _cv;
    Mutex* _mu;
};

#if defined USE_VISTA_API
typedef CondVarNew CondVar;
#else
typedef CondVarOld CondVar;
#endif

bool Snappy_Compress(const char* input, size_t input_length,
    std::string* output);
bool Snappy_Uncompress(const char* input_data, size_t input_length,
    std::string* output);

inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg) {
    return false;
}

}
}

#endif