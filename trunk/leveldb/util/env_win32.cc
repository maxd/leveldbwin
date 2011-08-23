

#include "../leveldb/env.h"
#include "../leveldb/slice.h"
#include "../port/port.h"
#include "../util/logging.h"
#include <atlbase.h>
#include <atlconv.h>

#include <Shlwapi.h>
#include <process.h>
#include <cstring>
#include <map>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <DbgHelp.h>
#include <algorithm>
#pragma comment(lib,"DbgHelp.lib")

#if defined DeleteFile
#undef DeleteFile
#endif
//Declarations
namespace leveldb
{

namespace Win32
{

typedef ATL::CA2WEX<MAX_PATH> MultiByteToWChar;
typedef ATL::CW2AEX<MAX_PATH> WCharToMultiByte;
typedef ATL::CA2AEX<MAX_PATH> MultiByteToAnsi;

std::string GetCurrentDir();
std::wstring GetCurrentDirW();

static const std::string CurrentDir = GetCurrentDir();
static const std::wstring CurrentDirW = GetCurrentDirW();

std::string& ModifyPath(std::string& path);
std::wstring& ModifyPath(std::wstring& path);

std::string GetLastErrSz();
std::wstring GetLastErrSzW();

class TaskRunner
{
public:
    struct Task
    {
        Task(void (*function_)(void*), void* arg_ = NULL) :
            function(function_),arg(arg_) { }
        void (*function)(void*);
        void* arg;
    };
    TaskRunner();
    ~TaskRunner();
    bool AddTask(const Task& task);
    bool AddTask( void (*function_)(void*), void* arg_ );
    bool SetThreadCount(size_t Count);
    size_t GetThreadCount();
    static const size_t DefaultThreadsCount = 5;

private:
    DISALLOW_COPY_AND_ASSIGN(TaskRunner);
    enum ThreadState
    {
        Unknown,
        Preparing,
        Working,
        Idle,
        ShutDown
    };
    static void _WorkingThreadProc(void* arg);
    static const size_t _DefaultWatingTime = 20;
    void _Init();
    void _UnInit();
    int _AddThread(int Count);
    int _DestroyThread(int Count);
    std::list<Task> _QueuedTasks;
    std::map<DWORD,ThreadState> _States;
    port::RWLock _i_states_lock;
    port::Mutex _i_tasks_lock;
    port::Event _OnAllRelease;
    port::Event _OnHasNewTask;
    bool _IsWorking;
};

template<typename T>
class SingletonHelper
{
public:
    SingletonHelper(bool bInitOnCreate = false)
    {
        if(bInitOnCreate)
            _p = new T();
    }
    ~SingletonHelper()
    {
        Delete();
    }
    static void Delete()
    {
        if(_p)
            delete _p;
    }
    static T* GetPointer()
    {
        if(!_p)
            _p = new T();
        return _p;
    }
    static T& GetReference()
    {
        if(!_p)
            _p = new T();
        return *_p;
    }
private:
   static T* _p;
};

}

class Win32SequentialFile : public SequentialFile
{
public:
    friend class Win32Env;
    virtual ~Win32SequentialFile();
    virtual Status Read(size_t n, Slice* result, char* scratch);
    virtual Status Skip(uint64_t n);
    BOOL isEnable();
    BOOL Init();
    void CleanUp();
private:
    Win32SequentialFile(const std::string& fname);
    std::string _filename;
    ::HANDLE _hFile;
};

class Win32RandomAccessFile : public RandomAccessFile
{
public:
    friend class Win32Env;
    virtual ~Win32RandomAccessFile();
    virtual Status Read(uint64_t offset, size_t n, Slice* result,char* scratch) const;
    BOOL Init(LPCWSTR path);
    BOOL isEnable();
    void CleanUp();
private:
    Win32RandomAccessFile(const std::string& fname);
    HANDLE _hFile;
    const std::string _filename;
};

class Win32WritableFile : public WritableFile
{
public:
    friend class Win32Env;
    virtual ~Win32WritableFile();
    virtual Status Append(const Slice& data);
    virtual Status Close();
    virtual Status Flush();
    virtual Status Sync();
private:
    Win32WritableFile(const std::string& fname, FILE* f);
    std::string _filename;
    FILE* _file;
};

class Win32FileLock : public FileLock
{
public:
    friend class Win32Env;
    virtual ~Win32FileLock();
    BOOL Init(LPCWSTR path);
    void CleanUp();
    BOOL isEnable();
private:
    Win32FileLock(const std::string& fname);
    HANDLE _hFile;
};

class Win32Env : public Env
{
public:
    Win32Env();
    virtual ~Win32Env();
    virtual Status NewSequentialFile(const std::string& fname,
        SequentialFile** result);

    virtual Status NewRandomAccessFile(const std::string& fname,
        RandomAccessFile** result);
    virtual Status NewWritableFile(const std::string& fname,
        WritableFile** result);

    virtual bool FileExists(const std::string& fname);

    virtual Status GetChildren(const std::string& dir,
        std::vector<std::string>* result);

    virtual Status DeleteFile(const std::string& fname);

    virtual Status CreateDir(const std::string& dirname);

    virtual Status DeleteDir(const std::string& dirname);

    virtual Status GetFileSize(const std::string& fname, uint64_t* file_size);

    virtual Status RenameFile(const std::string& src,
        const std::string& target);

    virtual Status LockFile(const std::string& fname, FileLock** lock);

    virtual Status UnlockFile(FileLock* lock);

    virtual void Schedule(
        void (*function)(void* arg),
        void* arg);

    virtual void StartThread(void (*function)(void* arg), void* arg);

    virtual Status GetTestDirectory(std::string* path);

    virtual void Logv(WritableFile* log, const char* format, va_list ap);

    virtual uint64_t NowMicros();

    virtual void SleepForMicroseconds(int micros);
private:
    Win32::TaskRunner _runner;
};

Win32::SingletonHelper<Win32Env> g_Env;
Win32Env* Win32::SingletonHelper<Win32Env>::_p = NULL;
}

//Implementations
namespace leveldb
{

namespace Win32
{

std::string GetCurrentDir()
{
    CHAR path[MAX_PATH];
    ::GetModuleFileNameA(::GetModuleHandleA(NULL),path,MAX_PATH);
    *strrchr(path,'\\') = 0;
    return std::string(path);
}

std::wstring GetCurrentDirW()
{
    WCHAR path[MAX_PATH];
    ::GetModuleFileNameW(::GetModuleHandleW(NULL),path,MAX_PATH);
    *wcsrchr(path,L'\\') = 0;
    return std::wstring(path);
}

std::string& ModifyPath(std::string& path)
{
    if(path[0] == '/' || path[0] == '\\'){
        path = CurrentDir + path;
    }
    std::replace(path.begin(),path.end(),'/','\\');

    return path;
}

std::wstring& ModifyPath(std::wstring& path)
{
    if(path[0] == L'/' || path[0] == L'\\'){
        path = CurrentDirW + path;
    }
    std::replace(path.begin(),path.end(),L'/',L'\\');
    return path;
}

std::string GetLastErrSz()
{
    LPVOID lpMsgBuf;
    FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
        );
    std::string Err = WCharToMultiByte((LPCWSTR)lpMsgBuf); 
    LocalFree( lpMsgBuf );
    return Err;
}

std::wstring GetLastErrSzW()
{
    LPVOID lpMsgBuf;
    FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
        );
    std::wstring Err = (LPCWSTR)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return Err;
}

void TaskRunner::_WorkingThreadProc( void* arg )
{
    TaskRunner* pThis = static_cast<TaskRunner*>(arg);
    std::map<DWORD,ThreadState>& States = pThis->_States;
    std::list<Task>& QueuedTasks = pThis->_QueuedTasks;
    port::RWLock& states_lock = pThis->_i_states_lock;
    port::Mutex& tasks_lock = pThis->_i_tasks_lock;
    port::Event& OnAllRelease = pThis->_OnAllRelease;
    port::Event& OnHasNewTask = pThis->_OnHasNewTask;
    bool& IsWorking = pThis->_IsWorking;

    const DWORD MyId = ::GetCurrentThreadId();

    states_lock.WriteLock();
    ThreadState CurrentState = (States[MyId] = Idle);
    states_lock.WriteUnlock();

    while(true){
        states_lock.ReadLock(); 
        //states_lock only lock in write mode when add or remove a value in States.
        if(States[MyId] == ShutDown){
            states_lock.ReadUnlock();
            break;
        }
        tasks_lock.Lock();
        if(!QueuedTasks.empty()){
            Task task = *(--QueuedTasks.end());
            QueuedTasks.pop_back();

            tasks_lock.Unlock();
            States[MyId] = Working;
            states_lock.ReadUnlock();

            task.function(task.arg);
        }else{
            tasks_lock.Unlock();
            States[MyId] = Idle;
            states_lock.ReadUnlock();

            OnHasNewTask.Wait(_DefaultWatingTime);
        }
    }

    states_lock.WriteLock();
    States.erase(MyId);
    if(!IsWorking && States.size() == 0){
        states_lock.WriteUnlock();
        OnAllRelease.Signal();
    } else
        states_lock.WriteUnlock();
}

TaskRunner::TaskRunner() : _IsWorking(true)
{
    _Init();
}

TaskRunner::~TaskRunner()
{
    _UnInit();
    _OnAllRelease.Wait();
}

void TaskRunner::_Init()
{
    _OnAllRelease.UnSignal();
    _OnHasNewTask.UnSignal();
    _AddThread(DefaultThreadsCount);
}

void TaskRunner::_UnInit()
{
    _IsWorking = false;
    _DestroyThread(GetThreadCount());
}

int TaskRunner::_AddThread( int Count )
{
    assert(Count > 0);
    size_t current = GetThreadCount();
    _i_states_lock.WriteLock();
    for(int i = 0 ;i < Count ; i++){
        HANDLE hThread = reinterpret_cast<HANDLE>(_beginthread(_WorkingThreadProc,0,this));
        _States[GetThreadId(hThread)] = Preparing;
    }
    _i_states_lock.WriteUnlock();
    return current + Count;
}

int TaskRunner::_DestroyThread( int Count )
{
    size_t current = GetThreadCount();
    if(current >= Count){
        _i_states_lock.WriteLock();
        size_t hasShutDown = 0;
        for(std::map<DWORD,ThreadState>::iterator i = _States.begin(); i != _States.end(); i++){
            if(i->second == Idle){
                i->second = ShutDown;
                hasShutDown++;
            }
            if(hasShutDown == Count)
                break;
        }
        if(hasShutDown < Count){
            for(std::map<DWORD,ThreadState>::iterator i = _States.begin(); i != _States.end(); i++){
                if(i->second != ShutDown){
                    i->second = ShutDown;
                    hasShutDown++;
                }
                if(hasShutDown == Count)
                    break;
            }
        }
        _i_states_lock.WriteUnlock();
        return current - Count;
    }else
        return -1;
}

size_t TaskRunner::GetThreadCount()
{
    size_t Count = 0;
    _i_states_lock.ReadLock();
    for(std::map<DWORD,ThreadState>::iterator i = _States.begin(); i != _States.end(); i++){
        if(i->second != ShutDown)
            Count++;
    }
    _i_states_lock.ReadUnlock();
    return Count;
}

bool TaskRunner::SetThreadCount( size_t Count )
{
    if(Count < 1)
        return false;
    size_t Current = GetThreadCount();
    if(Current > Count)
        _DestroyThread(Current - Count);
    else if(Current < Count)
        _AddThread(Count - Current);
    return true;
}

bool TaskRunner::AddTask( const Task& task )
{
    if(_IsWorking){
        _i_tasks_lock.Lock();
        _QueuedTasks.push_front(task);
        _i_tasks_lock.Unlock();
        _OnHasNewTask.Signal();
        return true;
    }else
        return false;
}

bool TaskRunner::AddTask( void (*function_)(void*), void* arg_ )
{
    return AddTask(Task(function_,arg_));
}

}



Env* Env::Default() 
{
    return Win32::SingletonHelper<Win32Env>::GetPointer();
}

Win32SequentialFile::Win32SequentialFile( const std::string& fname ) :
    _filename(fname),_hFile(NULL)
{
    Init();
}

Win32SequentialFile::~Win32SequentialFile()
{
    CleanUp();
}

Status Win32SequentialFile::Read( size_t n, Slice* result, char* scratch )
{
    Status sRet;
    DWORD hasRead = 0;
    if(_hFile && ReadFile(_hFile,scratch,n,&hasRead,NULL) ){
        *result = Slice(scratch,hasRead);
    } else {
        sRet = Status::IOError(_filename, Win32::GetLastErrSz() );
    }
    return sRet;
}

Status Win32SequentialFile::Skip( uint64_t n )
{
    Status sRet;
    LARGE_INTEGER Move,NowPointer;
    Move.QuadPart = n;
    if(!SetFilePointerEx(_hFile,Move,&NowPointer,FILE_CURRENT)){
        sRet = Status::IOError(_filename,Win32::GetLastErrSz());
    }
    return sRet;
}

BOOL Win32SequentialFile::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

BOOL Win32SequentialFile::Init()
{
    _hFile = CreateFileW(Win32::MultiByteToWChar(_filename.c_str() ),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    return _hFile ? TRUE : FALSE;
}

void Win32SequentialFile::CleanUp()
{
    if(_hFile){
        CloseHandle(_hFile);
        _hFile = NULL;
    }
}

Win32RandomAccessFile::Win32RandomAccessFile( const std::string& fname ) :
_filename(fname),_hFile(NULL)
{
    Init(Win32::MultiByteToWChar(fname.c_str() ) );
}

Win32RandomAccessFile::~Win32RandomAccessFile()
{
    CleanUp();
}

Status Win32RandomAccessFile::Read( uint64_t offset, size_t n, Slice* result,char* scratch ) const
{
    Status sRet;
    LARGE_INTEGER filePointer;
    filePointer.QuadPart = offset;
    DWORD hasRead;
    if(!(::SetFilePointerEx(_hFile,filePointer,NULL,FILE_BEGIN) && 
         ::ReadFile(_hFile,scratch,n,&hasRead,NULL) ) ){
            sRet = Status::IOError(_filename, "Could not preform read");
    }else
        *result = Slice(scratch,hasRead);
    return sRet;
}

BOOL Win32RandomAccessFile::Init( LPCWSTR path )
{
    BOOL bRet = FALSE;
    if(!_hFile)
        _hFile = ::CreateFileW(path,GENERIC_READ,0,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,NULL);
    if(!_hFile || _hFile == INVALID_HANDLE_VALUE )
        _hFile = NULL;
    else
        bRet = TRUE;
    return bRet;
}

BOOL Win32RandomAccessFile::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

void Win32RandomAccessFile::CleanUp()
{
    if(_hFile){
        ::CloseHandle(_hFile);
        _hFile = NULL;
    }
}

Win32WritableFile::Win32WritableFile( const std::string& fname, FILE* f ) :
    _filename(fname), _file(f)
{

}

Win32WritableFile::~Win32WritableFile()
{
    if(_file){
        fclose(_file);
        _file = NULL;
    }
}

Status Win32WritableFile::Append( const Slice& data )
{
    Status sRet;
    size_t size = fwrite(data.data(), 1, data.size(), _file);
    if (data.size() != size) {
        sRet = Status::IOError(_filename, strerror(errno));
    }
    return sRet;
}

Status Win32WritableFile::Close()
{
    Status sRet;
    if(_file){
       if( fclose(_file) != 0){
            sRet = Status::IOError(_filename, strerror(errno));
       }
       _file = NULL;
    }else
        sRet = Status::IOError(_filename, "this file has been closed.");
    return sRet;
}

Status Win32WritableFile::Flush()
{
    Status sRet;
    if(fflush(_file) != 0){
        sRet = Status::IOError(_filename, strerror(errno));
    }
    return sRet;
}

Status Win32WritableFile::Sync()
{
    Status sRet;
    if(fflush(_file) != 0 || _commit(fileno(_file)) ) {
        sRet = Status::IOError(_filename, strerror(errno));
    }
    return sRet;
}

Win32FileLock::Win32FileLock( const std::string& fname ) : _hFile(NULL)
{
    Init(Win32::MultiByteToWChar(fname.c_str() ) );
}

Win32FileLock::~Win32FileLock()
{
    CleanUp();
}

BOOL Win32FileLock::Init( LPCWSTR path )
{
    BOOL bRet = FALSE;
    if(!_hFile)
        _hFile = ::CreateFileW(path,0,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(!_hFile || _hFile == INVALID_HANDLE_VALUE ){
        _hFile = NULL;
        LPVOID lpMsgBuf;
        ::FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            0, // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
            );
        wprintf(L"\nCreate Lock Failed :\n%s\n",lpMsgBuf);
        ::LocalFree(lpMsgBuf);
    }
    else
        bRet = TRUE;
    return bRet;
}

void Win32FileLock::CleanUp()
{
    ::CloseHandle(_hFile);
    _hFile = NULL;
}

BOOL Win32FileLock::isEnable()
{
    return _hFile ? TRUE : FALSE;
}

bool Win32Env::FileExists(const std::string& fname)
{
    std::string path = fname;
    Win32::ModifyPath(path);
    return ::PathFileExistsW(Win32::MultiByteToWChar(path.c_str() ) ) ? true : false;
}

Status Win32Env::GetChildren(const std::string& dir, std::vector<std::string>* result)
{
    Status sRet;
    ::WIN32_FIND_DATAW wfd;
    std::string path = dir;
    Win32::ModifyPath(path);
    path += "\\*.*";
    ::HANDLE hFind = ::FindFirstFileW(
        Win32::MultiByteToWChar(path.c_str() ) ,&wfd);
    if(hFind && hFind != INVALID_HANDLE_VALUE){
        BOOL hasNext = TRUE;
        std::string child;
        while(hasNext){
            child = Win32::WCharToMultiByte(wfd.cFileName); 
            if(child != ".." && child != ".")  {
                result->push_back(child);
            }
            hasNext = ::FindNextFileW(hFind,&wfd);
        }
        ::FindClose(hFind);
    }
    else
        sRet = Status::IOError(dir,"Could not get children.");
    return sRet;
}

void Win32Env::SleepForMicroseconds( int micros )
{
    ::Sleep((micros + 999) /1000);
}

Status Win32Env::DeleteFile( const std::string& fname )
{
    Status sRet;
    std::string path = fname;
    Win32::ModifyPath(path);
    if(!::DeleteFileW(Win32::MultiByteToWChar(path.c_str() ) ) ){
        sRet = Status::IOError(path, "Could not delete file.");
    }
    return sRet;
}

Status Win32Env::GetFileSize( const std::string& fname, uint64_t* file_size )
{
    Status sRet;
    std::string path = fname;
    Win32::ModifyPath(path);
    HANDLE file = ::CreateFileW(Win32::MultiByteToWChar(path.c_str()),
        GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    LARGE_INTEGER li;
    if(::GetFileSizeEx(file,&li)){
        *file_size = (uint64_t)li.QuadPart;
    }else
        sRet = Status::IOError(path,"Could not get the file size.");
    CloseHandle(file);
    return sRet;
}

Status Win32Env::RenameFile( const std::string& src, const std::string& target )
{
    Status sRet;
    std::wstring src_path = Win32::MultiByteToWChar(src.c_str() );
    std::wstring target_path = Win32::MultiByteToWChar(target.c_str() );

    if(!MoveFile(Win32::ModifyPath(src_path).c_str(),
                 Win32::ModifyPath(target_path).c_str() ) ){
        DWORD err = GetLastError();
        if(err == 0x000000b7){
            if(!::DeleteFileW(target_path.c_str() ) )
                sRet = Status::IOError(src, "Could not rename file.");
            else if(!::MoveFileW(Win32::ModifyPath(src_path).c_str(),
                                 Win32::ModifyPath(target_path).c_str() ) )
                sRet = Status::IOError(src, "Could not rename file.");    
        }
    }
    return sRet;
}

Status Win32Env::LockFile( const std::string& fname, FileLock** lock )
{
    Status sRet;
    std::string path = fname;
    Win32::ModifyPath(path);
    Win32FileLock* _lock = new Win32FileLock(path);
    if(!_lock->isEnable()){
        delete _lock;
        *lock = NULL;
        sRet = Status::IOError(path, "Could not lock file.");
    }
    else
        *lock = _lock;
    return sRet;
}

Status Win32Env::UnlockFile( FileLock* lock )
{
    Status sRet;
    delete lock;
    return sRet;
}

void Win32Env::Schedule( void (*function)(void* arg), void* arg )
{
    _runner.AddTask(function,arg);
}

void Win32Env::StartThread( void (*function)(void* arg), void* arg )
{
    ::_beginthread(function,0,arg);
}

Status Win32Env::GetTestDirectory( std::string* path )
{
    Status sRet;
    WCHAR TempPath[MAX_PATH];
    ::GetTempPathW(MAX_PATH,TempPath);
    *path = Win32::WCharToMultiByte(TempPath);
    path->append("leveldb\\test\\");
    Win32::ModifyPath(*path);
    return sRet;
}

std::uint64_t Win32Env::NowMicros()
{
    return (std::uint64_t)(GetTickCount64()*1000);
}

Status Win32Env::CreateDir( const std::string& dirname )
{
    Status sRet;
    std::string path = Win32::MultiByteToAnsi(dirname.c_str() );
    if(path[path.length() - 1] != '\\'){
        path += '\\';
    }
    Win32::ModifyPath(path);
    if(!::MakeSureDirectoryPathExists( path.c_str() ) ){
        sRet = Status::IOError(dirname, "Could not create directory.");
    }
    return sRet;
}

Status Win32Env::DeleteDir( const std::string& dirname )
{
    Status sRet;
    std::wstring path = Win32::MultiByteToWChar(dirname.c_str() ) ;
    Win32::ModifyPath(path);
    if(!::RemoveDirectoryW( path.c_str() ) ){
        sRet = Status::IOError(dirname, "Could not delete directory.");
    }
    return sRet;
}

Status Win32Env::NewSequentialFile( const std::string& fname, SequentialFile** result )
{
    Status sRet;
    std::string path = fname;
    Win32::ModifyPath(path);
    Win32SequentialFile* pFile = new Win32SequentialFile(path);
    if(pFile->isEnable()){
        *result = pFile;
    }else {
        delete pFile;
        sRet = Status::IOError(path, Win32::GetLastErrSz());
    }
    return sRet;
}

Status Win32Env::NewRandomAccessFile( const std::string& fname, RandomAccessFile** result )
{
    Status sRet;
    std::string path = fname;
    Win32RandomAccessFile* pFile = new Win32RandomAccessFile(Win32::ModifyPath(path));
    if(!pFile->isEnable()){
        delete pFile;
        *result = NULL;
        sRet = Status::IOError(path,"Could not create random access file.");
    }else
        *result = pFile;
    return sRet;
}

Status Win32Env::NewWritableFile( const std::string& fname, WritableFile** result )
{
    Status sRet;
    std::string path = fname;
    Win32::ModifyPath(path);
    FILE* f = fopen(path.c_str(), "wb");
    if (f == NULL) {
        *result = NULL;
        sRet = Status::IOError(path, strerror(errno));
    } else
        *result = new Win32WritableFile(path, f);
    return sRet;
}

void Win32Env::Logv( WritableFile* log, const char* format, va_list ap )
{
    uint64_t thread_id = ::GetCurrentThreadId();

    // We try twice: the first time with a fixed-size stack allocated buffer,
    // and the second time with a much larger dynamically allocated buffer.
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
        char* base;
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
        }
        char* p = base;
        char* limit = base + bufsize;

        SYSTEMTIME st;
        GetLocalTime(&st);
        p += snprintf(p, limit - p,
            "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
            int(st.wYear),
            int(st.wMonth),
            int(st.wDay),
            int(st.wHour),
            int(st.wMinute),
            int(st.wMinute),
            int(st.wMilliseconds),
            static_cast<long long unsigned int>(thread_id));

        // Print the message
        if (p < limit) {
            va_list backup_ap;
            va_copy(backup_ap, ap);
            p += vsnprintf(p, limit - p, format, backup_ap);
            va_end(backup_ap);
        }

        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;       // Try again with larger buffer
            } else {
                p = limit - 1;
            }
        }

        // Add newline if necessary
        if (p == base || p[-1] != '\n') {
            *p++ = '\n';
        }

        assert(p <= limit);
        log->Append(Slice(base, p - base));
        log->Flush();
        if (base != buffer) {
            delete[] base;
        }
        break;
    }

}

Win32Env::Win32Env()
{

}

Win32Env::~Win32Env()
{

}

}