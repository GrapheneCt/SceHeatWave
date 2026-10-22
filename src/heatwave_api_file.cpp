/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @history
*   1.3   Change ::seek to return DataSource constants.
*/

#include "heatwave_api_file.h"
#include "heatwave_api_datasource.h"
#include <stdio.h>

#ifdef _WINDOWS
  #pragma warning(disable : 4996) /* Disable warning about fopen been unsafe */
#endif /* _WINDOWS */

#define SCE_HEATWAVE_NO_ELEMENTS_READ 0

namespace sce{
    namespace HeatWave{

        SceHwFileFunctions FileHandler::s_userOverrides;

        void* FileHandler::open(const char *strFilePath)
        {
            FILE *stream = NULL;

            if (FileHandler::s_userOverrides.pOpen)
            {
                return FileHandler::s_userOverrides.pOpen(strFilePath, "rb");
            }

            stream = fopen(strFilePath, "rb");
            return stream;
        }

        int32_t FileHandler::close(void *pStream)
        {
            FILE *stream = NULL;

            if (pStream == NULL)
            {
                return EOF;
            }

            if (FileHandler::s_userOverrides.pClose)
            {
                return FileHandler::s_userOverrides.pClose(pStream);
            }

            stream = (FILE*)pStream;
            return fclose(stream);
        }

        int32_t FileHandler::seek(void *pStream, int32_t uOffset, int32_t nOrigin)
        {
            FILE *stream = NULL;

            if (pStream == NULL)
            {
                return DataSource::knSeekFail;
            }

            if ((nOrigin != SEEK_SET) && (nOrigin != SEEK_CUR) && (nOrigin != SEEK_END))
            {
                return DataSource::knSeekFail;
            }

            if (FileHandler::s_userOverrides.pSeek)
            {
                return FileHandler::s_userOverrides.pSeek(pStream, uOffset, nOrigin);
            }

            stream = (FILE*)pStream;
            return fseek(stream, uOffset, nOrigin);
        }

        int32_t FileHandler::tell(void *pStream)
        {
            FILE *stream = NULL;

            if (pStream == NULL)
            {
                return EOF;
            }

            if (FileHandler::s_userOverrides.pTell)
            {
                return FileHandler::s_userOverrides.pTell(pStream);
            }

            stream = (FILE*)pStream;
            return (int32_t)ftell(stream);
        }

        uint32_t FileHandler::read(void *pPtr, uint32_t uSize, uint32_t uCount, void *pStream)
        {
            FILE *stream = NULL;

            if (pStream == NULL)
            {
                return SCE_HEATWAVE_NO_ELEMENTS_READ;
            }

            if (FileHandler::s_userOverrides.pRead)
            {
                return FileHandler::s_userOverrides.pRead(pPtr, uSize, uCount, pStream);
            }

            stream = (FILE*)pStream;
            return (uint32_t)fread(pPtr, uSize, uCount, stream);
        }

    } /* HeatWave */
} /* sce */
