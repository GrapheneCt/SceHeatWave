/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_platform.h"

#if defined(__PS3__) || defined(__ORBIS__)
  #define SCE_HEATWAVE_POSIX
  #include <pthread.h>
#endif

#if defined(SCE_HEATWAVE_CODEC_AT3) || defined(SCE_HEATWAVE_CODEC_MP3_PSVITA_SOFT)
#define SCE_HEATWAVE_THREAD_STACK_SIZE (64*1024) /* 64kB */
#else
#define SCE_HEATWAVE_THREAD_STACK_SIZE (32*1024) /* 32kB */
#endif

#ifdef SCE_HEATWAVE_PAF
#ifndef SCE_HEATWAVE_DYNAMIC
#error Option SCE_HEATWAVE_PAF does not support static build!
#endif
#ifdef __psp2__
/* Additional requirement for SCE_HEATWAVE_PAF option on PSP2:
 * 1. Add --no-standard-libraries parameter to the Project Properties->Linker->Command Line
 */
#pragma comment(lib, "libSceKernel_stub.a")
#pragma comment(lib, "_libScePafStdc_stub.a")
#pragma comment(lib, "libsnc.a")
#else
#error Option SCE_HEATWAVE_PAF is not supported for the selected platform!
#endif
#endif

namespace sce{
    namespace HeatWave{

        Thread::Thread()
            : m_isAborted(false),
            m_isCreated(false),
            m_threadFunc(NULL),
            m_userData(NULL)
        {
        }

        Thread::~Thread()
        {
        }

        void Thread::start(ThreadFunc pCall, void *pData)
        {
            if (pCall == NULL)
            {
                return;
            }
            if (pData == NULL)
            {
                return;
            }

            stop(); /* make sure there is no thread already running */

            m_isAborted = false;

            m_threadFunc    = pCall;
            m_userData      = pData;

            startPlatformThread();

            m_isCreated = true;
        }

        void Thread::stop()
        {
            if (m_isCreated == false)
            {
                return;
            }

            m_isAborted = true;
            stopPlatformThread();
            m_isCreated = false;

            m_userData      = NULL;
            m_threadFunc    = NULL;
        }

        bool Thread::isRunning()
        {
            return !m_isAborted;
        }

        int32_t Thread::run()
        {
            return m_threadFunc(m_userData);
        }

        #if defined(_WINDOWS)
        #define WINDOWS_LEAN_AND_MEAN
        #include <Windows.h>

        class MutexWin : public Mutex
        {
        public:
            MutexWin()
            : Mutex()
            {
                InitializeCriticalSection(&m_Mutex);
            }

            ~MutexWin()
            {
                DeleteCriticalSection(&m_Mutex);
            }

            void lock()
            {
                EnterCriticalSection(&m_Mutex);
            }

            void unlock()
            {
                LeaveCriticalSection(&m_Mutex);
            }

        private:
            CRITICAL_SECTION m_Mutex;
        };

        Mutex* _sceHeatWaveCreateMutex()
        {
            return new MutexWin();
        }


        class ThreadWin : public Thread
        {
        public:
            ThreadWin()
            : Thread()
            {
            }

            ~ThreadWin()
            {
            }

            void startPlatformThread()
            {
                m_threadHandle = CreateThread(NULL, 0, ThreadWin::_threadProcWin, this, 0, NULL);
            }

            void stopPlatformThread()
            {
                WaitForSingleObject(m_threadHandle, INFINITE); /* Wait whilst the m_threadFunc finishes */
                CloseHandle(m_threadHandle); /* Clean up */

                m_threadHandle = NULL;
            }

        private:
            static DWORD WINAPI _threadProcWin(LPVOID lpParam)
            {
                ThreadWin *threadHandle = (ThreadWin*)lpParam;
                if (threadHandle != NULL)
                {
                    return threadHandle->run();
                }

                return SCE_HEATWAVE_THREAD_FAILURE;
            }

            HANDLE m_threadHandle;
        };

        Thread* _sceHeatWaveCreateThread()
        {
            return new ThreadWin();
        }

        void _sceHeatWaveSleep(uint32_t uMilliSecs)
        {
            Sleep(uMilliSecs);
        }

        #elif defined(__psp2__)

        #include "kernel\threadmgr.h"

        class MutexPsp2 : public Mutex
        {
        public:
            MutexPsp2()
            : Mutex()
            {
                sceKernelCreateLwMutex(&m_mutex, "Mutex", SCE_KERNEL_MUTEX_ATTR_TH_FIFO | SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL);
            }

            ~MutexPsp2()
            {
                sceKernelDeleteLwMutex(&m_mutex);
            }

            void lock()
            {
                sceKernelLockLwMutex(&m_mutex, 1, NULL);
            }

            void unlock()
            {
                sceKernelUnlockLwMutex(&m_mutex, 1);
            }

        private:
            SceKernelLwMutexWork m_mutex;
        };

        Mutex* _sceHeatWaveCreateMutex()
        {
            return new MutexPsp2();
        }

        class ThreadPsp2 : public Thread
        {
        public:
            ThreadPsp2()
            : Thread(),
            m_threadHandle(-1)
            {

            }

            ~ThreadPsp2()
            {
            }

            void startPlatformThread()
            {
                m_threadHandle = sceKernelCreateThread("HeatWaveThread", ThreadPsp2::_threadProcPsp2, SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_HEATWAVE_THREAD_STACK_SIZE, 0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
                sceKernelStartThread(m_threadHandle, sizeof(ThreadPsp2), this);
            }

            void stopPlatformThread()
            {
                sceKernelWaitThreadEnd(m_threadHandle, NULL, NULL); /* Block for the thread to finish */
            }

        private:
            static SceInt32 _threadProcPsp2(SceSize argSize, void *pArgBlock)
            {
                ThreadPsp2 *threadHandle = (ThreadPsp2*)pArgBlock;
                SceInt32    threadResult = -1; /* default to failure */

                if (threadHandle != NULL)
                {
                    if(threadHandle->run() == SCE_HEATWAVE_THREAD_SUCCESS)
                    {
                        threadResult = 0;
                    }
                }

                sceKernelExitDeleteThread(threadResult);
                return threadResult;
            }

            SceUID m_threadHandle;
        };

        Thread* _sceHeatWaveCreateThread()
        {
            return new ThreadPsp2();
        }

        void _sceHeatWaveSleep(uint32_t uMilliSecs)
        {
            sceKernelDelayThread(uMilliSecs * 1000);
        }

        #elif defined(__PS3__)

        #include <sys\timer.h>

        void _sceHeatWaveSleep(uint32_t t)
        {
            sys_timer_usleep(t * 1000);
        }

        #elif defined(__ORBIS__)

        #include <time.h>
        #include <errno.h>

        void _sceHeatWaveSleep(uint32_t t)
        {
            SceKernelTimespec tv, rem;
            tv.tv_nsec = (t*1000000)%1000000000;
            tv.tv_sec = t/1000;

            while(sceKernelNanosleep(&tv, &rem) == -1 && errno == EINTR)
                tv = rem;
        }

        #endif

        #ifdef SCE_HEATWAVE_POSIX ///< __PS3__ || __ORBIS__

        class MutexPosix : public Mutex
        {
        public:
            MutexPosix()
                : Mutex()
            {
                pthread_mutexattr_t attrib;

                pthread_mutexattr_init(&attrib);
                pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE);
                pthread_mutex_init(&m_mutex, &attrib);
                pthread_mutexattr_destroy(&attrib);
            }

            ~MutexPosix()
            {
                pthread_mutex_destroy(&m_mutex);
            }

            void lock()
            {
                pthread_mutex_lock(&m_mutex);
            }

            void unlock()
            {
                pthread_mutex_unlock(&m_mutex);
            }

        private:
            pthread_mutex_t m_mutex;
        };

        class ThreadPosix : public Thread
        {
        public:
            ThreadPosix()
                : Thread(),
                m_threadHandle(NULL)
            {
            }

            ~ThreadPosix()
            {
            }

            void startPlatformThread()
            {
                pthread_attr_t attr;
                pthread_attr_init(&attr);

                pthread_attr_setstacksize(&attr, SCE_HEATWAVE_THREAD_STACK_SIZE);
                pthread_create(&m_threadHandle, &attr, ThreadPosix::_threadProcPosix, this);

                pthread_attr_destroy(&attr);
            }

            void stopPlatformThread()
            {
                pthread_join(m_threadHandle, NULL); /* Block whilst we wait for the thread to complete */
            }

        private:
            static void* _threadProcPosix(void *pData)
            {
                ThreadPosix *threadHandle = (ThreadPosix*)pData;
                if (threadHandle != NULL)
                {
                    threadHandle->run();
                }

                return NULL;
            }

            pthread_t m_threadHandle;
        };

        Mutex* _sceHeatWaveCreateMutex()
        {
            return new MutexPosix();
        }

        Thread* _sceHeatWaveCreateThread()
        {
            return new ThreadPosix();
        }

        #endif /* SCE_HEATWAVE_POSIX */

    } /* HeatWave */
} /* sce */

#ifdef SCE_HEATWAVE_PAF

extern "C" {

    extern int __cxa_atexit(void(*func)(void), void *arg, void *dso_handle);
    extern float sinf(float _Left);
    extern float cosf(float _Left);

    int _sceLdTlsRegisterModuleInfo()
    {
        return 0;
    }

    int _sceLdTlsUnregisterModuleInfo()
    {
        return 0;
    }

    int __at_quick_exit(void(*func)(void), void *arg, void *dso_handle)
    {
        return __cxa_atexit(func, 0, 0);
    }

    void _ZSt14_Xlength_errorPKc(const char *s)
    {
        //sceClibPrintf("_Xlength_error: %s", s);
        abort();
    }

    short _FDtest(float *p)
    {
        union
        {
            float f;
            uint32_t u;
        } v = { *p };

        uint32_t e = v.u & 0x7F800000u;
        uint32_t m = v.u & 0x007FFFFFu;

        if (e == 0x7F800000u)
        {
            return m ? 2 : 1;
        }

        if (e == 0)
        {
            return m ? -2 : 0;
        }

        return -1;
    }

#include <arm_neon.h>

    /*float sqrtf(float x)
    {
        float32x2_t d0 = vdup_n_f32(x);
        float32x2_t d1;
        float32x2_t d2;
        float32x2_t d3;

        // Fast inverse-square-root approximation
        d1 = d0;
        d0 = vrsqrte_f32(d0);

        d2 = vmul_f32(d0, d1);
        d3 = vrsqrts_f32(d2, d0);
        d0 = vmul_f32(d0, d3);

        d2 = vmul_f32(d0, d1);
        d3 = vrsqrts_f32(d2, d0);
        d0 = vmul_f32(d0, d3);

        // Fast reciprocal approximation
        d1 = vrecpe_f32(d0);
        d2 = vrecps_f32(d1, d0);
        d1 = vmul_f32(d1, d2);

        d2 = vrecps_f32(d1, d0);
        d0 = vmul_f32(d1, d2);


        return vget_lane_f32(d0, 0);
    }*/

    float sqrtf(float x)
    {
        __asm("vsqrt.f32 s0, s0");
        return x;
    }

    float sce_paf_sinf(float _Left)
    {
        return sinf(_Left);
    }

    float sce_paf_cosf(float _Left)
    {
        return cosf(_Left);
    }
}

#endif /* SCE_HEATWAVE_PAF */