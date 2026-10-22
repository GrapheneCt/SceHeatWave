/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_datasource.h
*
* @brief A source of data such as file or memory
*
* @history
*   1.1 Add ::clone() - create a new instance of the DataSource with a seperate handle to the source of data.
*   1.3 Fix for ::seek(0, SEEK_END) incorrectly returning an error code.
*       Add knSeekFail & knSeekSuccess constants.
*   1.4 Fix missing NULL check for allocation in FileSource::open.
*/

#ifndef _SCE_HEATWAVE_API_DATA_SOURCE_H_
#define _SCE_HEATWAVE_API_DATA_SOURCE_H_

#include "heatwave_api_memory.h"
#include "heatwave_api_file.h"
#include <stdio.h>
#include <string.h>

namespace sce{
    namespace HeatWave{

        /**
        * @brief Interface class to abstract data sources (File, memory)
        */
        class DataSource : public MemoryHandler
        {
        public:
            static const int32_t knSeekFail = -1;
            static const int32_t knSeekSuccess = 0;

            DataSource() {};
            virtual ~DataSource() {};

            virtual SceHwError clone(DataSource **ppCloneSourceOut) const = 0;

            virtual int32_t close() = 0;

            virtual int32_t tell() = 0;
            virtual int32_t seek(int32_t uOffset, int32_t nOrigin) = 0;
            virtual uint32_t read(void* pBuffer, uint32_t uSize, uint32_t uCount) = 0;
        };


        /**
        * @brief Data comes from a file resource
        */
        class FileSource : public DataSource
        {
        public:
            FileSource();
            ~FileSource();

            SceHwError clone(DataSource **pCloneSourceOut) const;

            bool open(const char *strFileName);

            int32_t tell();
            int32_t seek(int32_t uOffset, int32_t nOrigin);
            uint32_t read(void* pBuffer, uint32_t uSize, uint32_t uCount);

            int32_t close();

        private:
            void* m_fileHandle;
            char* m_filepath;
        };


        /**
        * @brief Data comes from a memory location
        */
        class MemorySource : public DataSource
        {
        public:
            MemorySource();
            SceHwError clone(DataSource **ppCloneSourceOut) const;

            bool open(char *pBuffer, uint32_t uBufferLen);

            int32_t tell();
            int32_t seek(int32_t uOffset, int32_t nOrigin);
            uint32_t read(void* pBuffer, uint32_t uSize, uint32_t uCount);

            int32_t close();

        private:
            char* m_buffer;
            uint32_t m_bufferLen;
            uint32_t m_offset;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_DATA_SOURCE_H_ */
