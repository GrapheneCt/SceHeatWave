/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_datasource.h"
#include "heatwave_api_audiostream.h"
#include "heatwave_api_errors.h"

#ifdef _WINDOWS
  #pragma warning(disable : 4996) /* Disable strncpy unsafe warning */
#endif /* _WINDOWS */

namespace sce{
    namespace HeatWave{

        FileSource::FileSource()
            : DataSource(),
            m_fileHandle(NULL),
            m_filepath(NULL)
        {
        }

        FileSource::~FileSource()
        {
            close();
        }

        /**
        * @brief Create a new FileSource with a distinct m_fileHandle to the file
        */
        SceHwError FileSource::clone(DataSource **ppCloneSourceOut) const
        {
            FileSource *cloneInstance = NULL;

            if (ppCloneSourceOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            cloneInstance = new FileSource();

            if (cloneInstance == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            if (!cloneInstance->open(this->m_filepath))
            {
                delete cloneInstance;
                return SCE_HEATWAVE_API_ERROR_FILE;
            }

            *ppCloneSourceOut = cloneInstance;
            return SCE_HEATWAVE_API_OK;
        }

        bool FileSource::open(const char *strFileName)
        {
            const size_t pathLength = strlen(strFileName) + 1;

            m_filepath = (char*)MemoryHandler::mallocCb(pathLength);
            if (m_filepath == NULL)
            {
                return false;
            }
            strncpy(m_filepath, strFileName, pathLength);

            m_fileHandle = FileHandler::open(m_filepath);
            return m_fileHandle != NULL;
        }

        uint32_t FileSource::tell()
        {
            return FileHandler::tell(m_fileHandle);
        }

        int32_t FileSource::seek(uint32_t uOffset, int32_t nOrigin)
        {
            return FileHandler::seek(m_fileHandle, uOffset, nOrigin);
        }

        uint32_t FileSource::read(void* pBuffer, uint32_t uSize, uint32_t uCount)
        {
            return FileHandler::read(pBuffer, uSize, uCount, m_fileHandle);
        }

        int32_t FileSource::close()
        {
            int32_t res = 0;

            if (m_fileHandle == NULL)
            {
                return 0;
            }

            if (m_filepath != NULL)
            {
                MemoryHandler::freeCb(m_filepath);
                m_filepath = NULL;
            }

            res = FileHandler::close(m_fileHandle);
            m_fileHandle = NULL;
            return res;
        }


        MemorySource::MemorySource()
            : DataSource(),
            m_buffer(NULL),
            m_bufferLen(0),
            m_offset(0)
        {
        }

        /**
        * @brief Create a new MemorySource pointing to m_buffer
        */
        SceHwError MemorySource::clone(DataSource **ppCloneSourceOut) const
        {
            MemorySource *cloneInstance = NULL;

            if (ppCloneSourceOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            cloneInstance = new MemorySource();
            if (cloneInstance == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            if (!cloneInstance->open(this->m_buffer, this->m_bufferLen))
            {
                delete cloneInstance;
                return SCE_HEATWAVE_API_ERROR_FILE;
            }

            *ppCloneSourceOut = cloneInstance;
            return SCE_HEATWAVE_API_OK;
        }

        bool MemorySource::open(char *pBuffer, uint32_t uBufferLen)
        {
            if (pBuffer == NULL || uBufferLen == 0)
            {
                m_buffer     = NULL; /* Make sure we are in a state where other functions will fail */
                m_bufferLen  = 0;
                m_offset     = 0;

                return false;
            }

            m_buffer    = pBuffer;
            m_bufferLen = uBufferLen;
            m_offset    = 0;

            return true;
        }

        uint32_t MemorySource::tell()
        {
            return m_offset;
        }

        int32_t MemorySource::seek(uint32_t uOffset, int32_t nOrigin)
        {
            uint32_t newOffset = 0;

            if (nOrigin == SEEK_SET)
            {
                newOffset = uOffset;
            }
            else if (nOrigin == SEEK_CUR)
            {
                newOffset = m_offset + uOffset;
            }
            else if (nOrigin == SEEK_END)
            {
                newOffset = m_bufferLen - uOffset;
            }
            else
            {
                return knSeekFail; ///< Unknown seek type.
            }

            if (newOffset > m_bufferLen)
            {
                return knSeekFail; ///< Attempting to seek beyond the end of the memory block.
            }

            m_offset = newOffset;
            return knSeekSuccess;
        }

        uint32_t MemorySource::read(void* pBuffer, uint32_t uSize, uint32_t uCount)
        {
            uint32_t bytesize = uSize * uCount;
            if (m_bufferLen - m_offset < bytesize)
            {
                bytesize = m_bufferLen - m_offset;
            }
            MemoryHandler::memcpyCb(pBuffer, m_buffer + m_offset, bytesize);
            m_offset += bytesize;
            return bytesize / uSize;
        }

        int32_t MemorySource::close()
        {
            m_buffer    = NULL;
            m_bufferLen = 0;
            m_offset    = 0;

            return 0;
        }

    } /* HeatWave */
} /* sce */
