/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "heatwave_api_ringbuffer.h"

#ifndef UINT32_MAX
  #define UINT32_MAX    0xffffffff
#endif /* UINT32_MAX */

namespace sce{
    namespace HeatWave{

        RingBuffer::RingBuffer(uint32_t uSizeBytes)
            : MemoryHandler(),
            m_readAvailable(0)
        {
            if (uSizeBytes > UINT32_MAX) /* Guarantee the pointer arithmetic is safe to cast to 32 on x64 */
            {
                uSizeBytes = UINT32_MAX;
            }

            m_dataStart = (char*)MemoryHandler::mallocCb(uSizeBytes);
            m_dataEnd = m_dataStart + uSizeBytes;

            m_read = m_dataStart;
            m_write = m_read;
        }

        RingBuffer::~RingBuffer()
        {
            if (m_dataStart != NULL)
            {
                MemoryHandler::freeCb(m_dataStart);
            }
            m_dataStart = NULL;
        }

        /**
        * @brief Make uReadSizeBytes of data available for reading
        */
        uint32_t RingBuffer::lockRead(uint32_t uReadSizeBytes, BitStream *pBuffer1, BitStream *pBuffer2)
        {
            const uint32_t totalReadSize = availableRead() > uReadSizeBytes ? uReadSizeBytes : availableRead();

            if (pBuffer1 == NULL || pBuffer2 == NULL)
            {
                return 0;
            }

            if (totalReadSize == 0)
            {
                return 0;
            }

            pBuffer1->pData = NULL;
            pBuffer2->pData = NULL;
            pBuffer1->uSizeBytes = 0;
            pBuffer2->uSizeBytes = 0;

            if (m_read > m_write)
            {
                pBuffer1->pData = m_read;
                pBuffer1->uSizeBytes = (uint32_t)(m_dataEnd - m_read);

                if (pBuffer1->uSizeBytes > totalReadSize)
                {
                    pBuffer1->uSizeBytes = totalReadSize;
                }
                else /* Read takes us past the end, wrap around to start */
                {
                    pBuffer2->pData = m_dataStart;
                    pBuffer2->uSizeBytes = totalReadSize - pBuffer1->uSizeBytes;
                }
            }
            else
            {
                pBuffer1->pData = m_read;
                pBuffer1->uSizeBytes = totalReadSize;
            }

            return totalReadSize;
        }

        /**
        * @brief Increment the read position by uNumBytes
        */
        uint32_t RingBuffer::unlockRead(uint32_t uNumBytes)
        {
            const uint32_t totalReadSize = availableRead() > uNumBytes ? uNumBytes : availableRead();

            m_read = m_read + totalReadSize;
            if (m_read >= m_dataEnd)
            {
                m_read = m_read - (m_dataEnd - m_dataStart);
            }

            m_readAvailable -= totalReadSize;
            return totalReadSize;
        }

        /**
        * @brief Return the number of bytes of read data contained
        */
        uint32_t RingBuffer::availableRead()
        {
            return m_readAvailable;
        }

        /**
        * @brief Make uWriteSizeBytes of data available for writing to
        */
        uint32_t RingBuffer::lockWrite(uint32_t uWriteSizeBytes, BitStream *pBuffer1, BitStream *pBuffer2)
        {
            const uint32_t totalWriteSize = availableWrite() > uWriteSizeBytes ? uWriteSizeBytes : availableWrite();

            if (pBuffer1 == NULL || pBuffer2 == NULL)
            {
                return 0;
            }
            if (totalWriteSize == 0)
            {
                return 0;
            }

            pBuffer1->pData = NULL;
            pBuffer2->pData = NULL;
            pBuffer1->uSizeBytes = 0;
            pBuffer2->uSizeBytes = 0;

            if (m_write > m_read)
            {
                pBuffer1->pData = m_write;
                pBuffer1->uSizeBytes = (uint32_t)(m_dataEnd - m_write);

                if (pBuffer1->uSizeBytes > totalWriteSize)
                {
                    pBuffer1->uSizeBytes = totalWriteSize;
                }
                else /* Write goes past the end, wrap to the start */
                {
                    pBuffer2->pData = m_dataStart;
                    pBuffer2->uSizeBytes = totalWriteSize - pBuffer1->uSizeBytes;
                }
            }
            else
            {
                pBuffer1->pData = m_write;
                pBuffer1->uSizeBytes = totalWriteSize;
            }

            return totalWriteSize;
        }

        /**
        * @brief Increment the write position by uNumBytes
        */
        uint32_t RingBuffer::unlockWrite(uint32_t uNumBytes)
        {
            const uint32_t totalWriteSize = availableWrite() > uNumBytes ? uNumBytes : availableWrite();

            m_write = m_write + totalWriteSize;
            if (m_write >= m_dataEnd)
            {
                m_write = m_write - (m_dataEnd - m_dataStart);
            }

            m_readAvailable += totalWriteSize;
            return totalWriteSize;
        }

        /**
        * @brief Return the number of bytes available for write
        */
        uint32_t RingBuffer::availableWrite()
        {
            return (uint32_t)(m_dataEnd - m_dataStart) - availableRead();
        }

        /**
        * @brief Clear the contents, set to an empty state
        */
        void RingBuffer::reset()
        {
            m_read = m_dataStart;
            m_write = m_read;
            m_readAvailable = 0;

            MemoryHandler::memsetCb(m_dataStart, 0, m_dataEnd - m_dataStart);
        }

    } /* HeatWave */
} /* sce */
