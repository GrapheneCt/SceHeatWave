/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_ringbuffer.h
*
* @brief A basic circular buffer
*
* @history
*   1.1 File added.
*   1.4 Fix memory leak; missing free of memory previously allocated in constructor.
*/

#ifndef _SCE_HEATWAVE_API_RING_BUFFER_H_
#define _SCE_HEATWAVE_API_RING_BUFFER_H_

#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        typedef struct BitStream : public MemoryHandler
        {
            char *pData;
            uint32_t uSizeBytes;
        } BitStream;

        class RingBuffer : public MemoryHandler
        {
        public:
            RingBuffer(uint32_t uSizeBytes);
            ~RingBuffer();

            uint32_t lockRead(uint32_t uReadSizeBytes, BitStream *pBuffer1, BitStream *pBuffer2);
            uint32_t unlockRead(uint32_t uNumBytes);
            uint32_t availableRead();

            uint32_t lockWrite(uint32_t uWriteSizeBytes, BitStream *pBuffer1, BitStream *pBuffer2);
            uint32_t unlockWrite(uint32_t uNumBytes);
            uint32_t availableWrite();

            void reset();

        private:
            char* m_dataStart;
            char* m_dataEnd;
            char* m_read;
            char* m_write;
            uint32_t m_readAvailable;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_RING_BUFFER_H_ */
