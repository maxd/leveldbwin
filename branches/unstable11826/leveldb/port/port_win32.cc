
#include "../port/port_win32.h"

#include <stack>
#include <cassert>
#include <algorithm>

#if defined USE_SNAPPY
#include "..\..\snappy\snappy.h"

#if defined _DEBUG
#pragma comment(lib,"..\\Debug\\snappy.lib")
#else
#pragma comment(lib,"..\\Release\\snappy.lib")
#endif

#endif

namespace leveldb
{

namespace port
{

bool Snappy_Compress(const char* input, size_t input_length,
    std::string* output) {
#if defined(USE_SNAPPY)
        output->resize(snappy::MaxCompressedLength(input_length));
        size_t outlen;
        snappy::RawCompress(input, input_length, &(*output)[0], &outlen);
        output->resize(outlen);
        return true;
#else
        return false;
#endif
}

bool Snappy_Uncompress(const char* input_data, size_t input_length,
    std::string* output) {
#if defined(USE_SNAPPY)
        size_t ulength;
        if (!snappy::GetUncompressedLength(input_data, input_length, &ulength)) {
            return false;
        }
        output->resize(ulength);
        return snappy::RawUncompress(input_data, input_length, &(*output)[0]);
#else
        return false;
#endif
}

Event::Event( bool bSignal,bool ManualReset ) : _hEvent(NULL)
{
    _hEvent = ::CreateEvent(NULL,ManualReset,bSignal,NULL);
}

Event::~Event()
{
    Signal();
    CloseHandle(_hEvent);
}

void Event::Wait(DWORD Milliseconds /*= INFINITE*/ )
{
    WaitForSingleObject(_hEvent,Milliseconds);
}

void Event::Signal()
{
    SetEvent(_hEvent);
}

void Event::UnSignal()
{
    ResetEvent(_hEvent);
}

Mutex::Mutex()
{
    InitializeCriticalSection(&_cs);
}

Mutex::~Mutex()
{
    DeleteCriticalSection(&_cs);
}

void Mutex::Lock()
{
    EnterCriticalSection(&_cs);
}

void Mutex::Unlock()
{
    LeaveCriticalSection(&_cs);
}

void Mutex::AssertHeld()
{
    assert( _cs.OwningThread == reinterpret_cast<HANDLE>(GetCurrentThreadId() ) );
        
}

BOOL Mutex::TryLock()
{
    return TryEnterCriticalSection(&_cs);
}


CondVarOld::CondVarOld( Mutex* mu ) : 
    _user_lock(mu),
    _run_state(RUNNING),
    _alloced_handles_count(0)
{

}

CondVarOld::~CondVarOld()
{
    AutoLock auto_lock(_internal_lock);
    _run_state = SHUTDOWN;

    if (_recycling_handles.size() != _alloced_handles_count ) {
        AutoUnlock auto_unlock(_internal_lock);
        this->SignalAll(); 
        while(_recycling_handles.size() != _alloced_handles_count )
            Sleep(10);
    }
    CloseRecyclingHandles();
}

HANDLE CondVarOld::GetWaitingHandle()
{
    HANDLE handle = NULL;
    if(_recycling_handles.empty()){
        handle = CreateEvent(NULL,FALSE,FALSE,NULL);
        _alloced_handles_count++;
    }else{
        handle = *_recycling_handles.end();
        _recycling_handles.pop_back();
    }
    _waiting_handles.push_back(handle);
    return handle;
}

void CondVarOld::CloseRecyclingHandles()
{
    while (!_recycling_handles.empty()){
        SetEvent(*_recycling_handles.end());
        _recycling_handles.pop_back();
    }
}

void CondVarOld::timedWait(DWORD dwMilliseconds)
{
    HANDLE handle = NULL;
    {
        AutoLock auto_lock(_internal_lock);
        if (RUNNING != _run_state) return;
        handle = GetWaitingHandle();
    }
        
    {
        AutoUnlock unlock(*_user_lock);
        WaitForSingleObject(handle,dwMilliseconds);
        AutoLock auto_lock(_internal_lock);
        _recycling_handles.push_back(handle);
    } 
}

void CondVarOld::Wait()
{
    timedWait(INFINITE);
}

void CondVarOld::SignalAll()
{
    std::stack<HANDLE> handles;
    {
        AutoLock auto_lock(_internal_lock);
        if (_waiting_handles.empty())
            return;
        while (!_waiting_handles.empty()){
            handles.push(*_waiting_handles.end());
            _waiting_handles.pop_back();
        }
    }
    while (!handles.empty()) {
        SetEvent(handles.top());
        handles.pop();
    }
}

void CondVarOld::Signal()
{
    HANDLE handle;
    {
        AutoLock auto_lock(_internal_lock);
        if (_waiting_handles.empty())
            return; 
        handle = *_waiting_handles.end();
        _waiting_handles.pop_back();
    }  
    SetEvent(handle);
}




CondVarNew::CondVarNew( Mutex* mu ) : _mu(mu)
{
    InitializeConditionVariable(&_cv);
}

CondVarNew::~CondVarNew()
{
    WakeAllConditionVariable(&_cv);
}

void CondVarNew::Wait()
{
    SleepConditionVariableCS(&_cv,&_mu->_cs,INFINITE);
}

void CondVarNew::Signal()
{
    WakeConditionVariable(&_cv);
}

void CondVarNew::SignalAll()
{
    WakeAllConditionVariable(&_cv);
}


RWLock::RWLock()
{
    InitializeSRWLock(&_srw);
}

RWLock::~RWLock()
{

}

void RWLock::ReadLock()
{
    AcquireSRWLockShared(&_srw);
}

void RWLock::ReadUnlock()
{
    ReleaseSRWLockShared(&_srw);
}

void RWLock::WriteLock()
{
    AcquireSRWLockExclusive(&_srw);
}

void RWLock::WriteUnlock()
{
    ReleaseSRWLockExclusive(&_srw);
}

}


}