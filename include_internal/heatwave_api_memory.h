/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_memory.h
*
* @brief All memory manipulation (malloc, free, etc) passes through here to allow implementation of user overrides
*/

#ifndef _SCE_HEATWAVE_API_MEMORY_H_
#define _SCE_HEATWAVE_API_MEMORY_H_

#include <stdlib.h>
#include <new>
#include "heatwave_api_types.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Base class to allow overloading of the memory functions that the api uses
        * Don't want to override the global versions of new\delete in case we cause conflicts with other libs or something the api user has already setup...
        * All classes in the api should inherit from this so that all memory allocation by the api passes through here
        */
        class MemoryHandler
        {
        public:
            void* operator new(size_t sz) throw();
            void  operator delete(void *pMem, size_t sz);

            void* operator new[](size_t sz) throw();
            void  operator delete[](void* pMem, size_t sz);

            /**
            * @brief Custom memory handlers so that we can hook in user supplied versions
            */
            static void* mallocCb(size_t pSize);
            static void* reallocCb(void *pMem, size_t pSize);
            static void* callocCb(size_t pNum, size_t pSize);
            static void* memalignCb(size_t pAlign, size_t pSize);
            static void  freeCb(void* pMem);

            static void  memsetCb(void* pMem, int32_t pVal, size_t pSize);

            static void* memcpyCb(void* pDest, void* pSource, size_t pSize);

            static void* memmoveCb(void* pDest, void* pSource, size_t pSize);

            /**
            * @brief User overrides for mallocCb\freeCb
            */
            static SceHwMemoryFunctions s_userOverrides;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_MEMORY_H_ */
