/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_platform.h
*
* @brief Wrapper to hide away platform specific versions of threads, mutex and sleep
*/

#ifndef _SCE_HEATWAVE_API_PLATFORM_H_
#define _SCE_HEATWAVE_API_PLATFORM_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        #define SCE_HEATWAVE_THREAD_SUCCESS 0
        #define SCE_HEATWAVE_THREAD_FAILURE 1

        class Mutex;
        class Thread;

        extern void     _sceHeatWaveSleep(uint32_t uMilliSecs);
        extern Mutex*   _sceHeatWaveCreateMutex();
        extern Thread*  _sceHeatWaveCreateThread();

        class Mutex : public MemoryHandler
        {
        public:
            Mutex() {};
            virtual ~Mutex() {};

            virtual void lock() =0;
            virtual void unlock() =0;
        };


        typedef int32_t(*ThreadFunc)(void *pUserData);

        class Thread : public MemoryHandler
        {
        public:
            Thread();
            virtual ~Thread();

            void start(ThreadFunc pCall, void *pData); ///< Launch pCall on a thread and pass pData as the user data
            void stop();

            bool isRunning();

        protected:
            virtual void startPlatformThread() = 0;
            virtual void stopPlatformThread() = 0;

            int32_t run();

            volatile bool   m_isAborted;
            bool            m_isCreated;

            ThreadFunc  m_threadFunc;
            void        *m_userData;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_PLATFORM_H_ */
