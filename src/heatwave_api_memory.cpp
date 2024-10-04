/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_memory.h"
#include <string.h>
#include <stdio.h>

namespace sce{
    namespace HeatWave{

        SceHwMemoryFunctions MemoryHandler::s_userOverrides;

        void* MemoryHandler::operator new(size_t sz) throw()
        {
            void *mem = NULL;

            if (sz == 0)
            {
                sz = 1; /* 0 allocations should return a valid pointer */
            }

            mem = MemoryHandler::mallocCb(sz);
            return mem;
        }

        void MemoryHandler::operator delete(void *pMem, size_t sz)
        {
            if (pMem != NULL)
            {
                MemoryHandler::freeCb(pMem);
            }
        }

        void* MemoryHandler::operator new[](size_t sz) throw()
        {
            void *mem = NULL;

            if (sz == 0)
            {
                sz = 1; /* 0 allocations should return a valid pointer */
            }

            mem = MemoryHandler::mallocCb(sz);
            return mem;
        }

        void MemoryHandler::operator delete[](void* pMem, size_t sz)
        {
            if (pMem != NULL)
            {
                MemoryHandler::freeCb(pMem);
            }
        }

        void* MemoryHandler::mallocCb(size_t pSize)
        {
            if (MemoryHandler::s_userOverrides.pAlloc)
            {
                return MemoryHandler::s_userOverrides.pAlloc(pSize);
            }
            return malloc(pSize);
        }

        void* MemoryHandler::reallocCb(void *pMem, size_t pSize)
        {
            if (MemoryHandler::s_userOverrides.pRealloc)
            {
                return MemoryHandler::s_userOverrides.pRealloc(pMem, pSize);
            }
            return realloc(pMem, pSize);
        }

        void* MemoryHandler::callocCb(size_t pNum, size_t pSize)
        {
            if (MemoryHandler::s_userOverrides.pCalloc)
            {
                return MemoryHandler::s_userOverrides.pCalloc(pNum, pSize);
            }
            return calloc(pNum, pSize);
        }

        void* MemoryHandler::memalignCb(size_t nAlign, size_t nSize)
        {
            if (MemoryHandler::s_userOverrides.pAlignedAlloc)
            {
                return MemoryHandler::s_userOverrides.pAlignedAlloc(nAlign, nSize);
            }
#ifdef _WINDOWS
            return MemoryHandler::mallocCb(nSize + (nAlign - (nSize%nAlign)));
#else
            return memalign(nAlign, nSize);
#endif /* _WINDOWS */
        }

        void MemoryHandler::freeCb(void* pMem)
        {
            if (MemoryHandler::s_userOverrides.pFree)
            {
                return MemoryHandler::s_userOverrides.pFree(pMem);
            }
            else
            {
                free(pMem);
            }
        }

        void MemoryHandler::memsetCb(void* pMem, int32_t pVal, size_t pSize)
        {
            memset(pMem, pVal, pSize);
        }

        void* MemoryHandler::memcpyCb(void* pDest, void* pSource, size_t pSize)
        {
            return memcpy(pDest, pSource, pSize);
        }

    } /* HeatWave */
} /* sce */
