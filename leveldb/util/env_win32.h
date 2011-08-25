

#ifndef STORAGE_LEVELDB_UTIL_ENV_WIN32_H_
#define STORAGE_LEVELDB_UTIL_ENV_WIN32_H_

#include <atlbase.h>
#include <atlconv.h>
#include <map>

#if defined DeleteFile
#undef DeleteFile
#endif

#include "../leveldb/env.h"
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
            static const size_t DefaultThreadsCount = 3;

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
            LONG _IsWorking;
        };

        template<typename T>
        class SingletonHelper
        {
        public:
            static void Delete()
            {
                if(_p){
                    delete _p;
                    _p = NULL;
                }
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
            SingletonHelper(bool bInitOnCreate = false)
            {
                if(bInitOnCreate && !_p)
                    _p = new T();
            }
            ~SingletonHelper()
            {
                Delete();
            }
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
}

#endif