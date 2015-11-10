/**
 * @file    TestCase_Core_Thread_Lock.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2013/05/03
 * @version 1.0
 *
 * @brief
 */

#include "core/thread/TestCase_Core_Thread_Lock.h"

static const int __g_threads_num = 5;
static LLBC_SimpleLock __g_outLock;

struct __LLBC_Thread_Test_Arg
{
    LLBC_ILock *lock;
    uint64 val;
};

static int ThreadProc(void *arg)
{
    int threadIndex = *(reinterpret_cast<int *>(arg));
    __LLBC_Thread_Test_Arg *threadArg;
    memcpy(&threadArg, reinterpret_cast<char *>(arg) + sizeof(int), sizeof(__LLBC_Thread_Test_Arg *));

    __g_outLock.Lock();
    std::cout <<"thread [" <<threadIndex <<"]" <<" begin call lock" <<std::endl;
    __g_outLock.Unlock();
    for(int i = 0; i < 500000; i ++)
    {
        threadArg->lock->Lock();
        threadArg->val ++;
        threadArg->lock->Unlock();
    }

    __g_outLock.Lock();
    std::cout <<"thread [" <<threadIndex <<"]" <<" end call lock" <<std::endl;
    __g_outLock.Unlock();

    free(arg);

    return 0;
}

TestCase_Core_Thread_Lock::TestCase_Core_Thread_Lock()
{
}

TestCase_Core_Thread_Lock::~TestCase_Core_Thread_Lock()
{
}

int TestCase_Core_Thread_Lock::Run(int argc, char *argv[])
{
    std::cout <<"core/thread/lock test:" <<std::endl;


    __LLBC_Thread_Test_Arg *threadArg = new __LLBC_Thread_Test_Arg;
    LLBC_NativeThreadHandle handles[__g_threads_num] = {LLBC_INVALID_NATIVE_THREAD_HANDLE};

    // SimpleLock test.
    std::cout <<"Test SimpleLock ..." <<std::endl;
    threadArg->lock = new LLBC_SimpleLock;
    threadArg->val = 0;
    for(int i = 0; i < __g_threads_num; i ++)
    {
        char *buf = reinterpret_cast<char *>(malloc(sizeof(int) + sizeof(__LLBC_Thread_Test_Arg *)));

        *(reinterpret_cast<int *>(buf)) = i;
        memcpy(buf + sizeof(int), &threadArg, sizeof(__LLBC_Thread_Test_Arg *));
        LLBC_CreateThread(&handles[i], &ThreadProc, buf);
    }

    for(int i = 0; i < __g_threads_num; i ++)
    {
        LLBC_JoinThread(handles[i]);
    }
    delete threadArg->lock;
    std::cout <<"\t OK, value: " <<threadArg->val <<std::endl;

    // RecursiveLock test.
    std::cout <<"Test RecursiveLock ..." <<std::endl;
    threadArg->lock = new LLBC_RecursiveLock;
    threadArg->val = 0;
    for(int i = 0; i < __g_threads_num; i ++)
    {
        char *buf = reinterpret_cast<char *>(malloc(sizeof(int) + sizeof(__LLBC_Thread_Test_Arg *)));

        *(reinterpret_cast<int *>(buf)) = i;
        memcpy(buf + sizeof(int), &threadArg, sizeof(__LLBC_Thread_Test_Arg *));
        LLBC_CreateThread(&handles[i], &ThreadProc, buf);
    }

    for(int i = 0; i < __g_threads_num; i ++)
    {
        LLBC_JoinThread(handles[i]);
    }
    delete threadArg->lock;
    std::cout <<"\t OK, value: " <<threadArg->val <<std::endl;

    // FastLock test.
    std::cout <<"Test FastLock ..." <<std::endl;
    threadArg->lock = new LLBC_FastLock;
    threadArg->val = 0;
    for(int i = 0; i < __g_threads_num; i ++)
    {
        char *buf = reinterpret_cast<char *>(malloc(sizeof(int) + sizeof(__LLBC_Thread_Test_Arg *)));

        *(reinterpret_cast<int *>(buf)) = i;
        memcpy(buf + sizeof(int), &threadArg, sizeof(__LLBC_Thread_Test_Arg *));
        LLBC_CreateThread(&handles[i], &ThreadProc, buf);
    }

    for(int i = 0; i < __g_threads_num; i ++)
    {
        LLBC_JoinThread(handles[i]);
    }
    delete threadArg->lock;
    std::cout <<"\t OK, value: " <<threadArg->val <<std::endl;

    // SpinLock test.
    std::cout <<"Test SpinLock ..." <<std::endl;
    threadArg->lock = new LLBC_SpinLock;
    threadArg->val = 0;
    for(int i = 0; i < __g_threads_num; i ++)
    {
        char *buf = reinterpret_cast<char *>(malloc(sizeof(int) + sizeof(__LLBC_Thread_Test_Arg *)));

        *(reinterpret_cast<int *>(buf)) = i;
        memcpy(buf + sizeof(int), &threadArg, sizeof(__LLBC_Thread_Test_Arg *));
        LLBC_CreateThread(&handles[i], &ThreadProc, buf);
    }

    for(int i = 0; i < __g_threads_num; i ++)
    {
        LLBC_JoinThread(handles[i]);
    }
    delete threadArg->lock;
    std::cout <<"\t OK, value: " <<threadArg->val <<std::endl;

    // DummyLock test.
    std::cout <<"Test DummyLock ..." <<std::endl;
    threadArg->lock = new LLBC_DummyLock;
    threadArg->val = 0;
    for(int i = 0; i < __g_threads_num; i ++)
    {
        char *buf = reinterpret_cast<char *>(malloc(sizeof(int) + sizeof(__LLBC_Thread_Test_Arg *)));

        *(reinterpret_cast<int *>(buf)) = i;
        memcpy(buf + sizeof(int), &threadArg, sizeof(__LLBC_Thread_Test_Arg *));
        LLBC_CreateThread(&handles[i], &ThreadProc, buf);
    }

    for(int i = 0; i < __g_threads_num; i ++)
    {
        LLBC_JoinThread(handles[i]);
    }
    delete threadArg->lock;
    std::cout <<"\t OK, value: " <<threadArg->val <<std::endl;

    delete threadArg;

    std::cout <<"Press any key to continue ... ..." <<std::endl;
    getchar();

    return 0;
}
