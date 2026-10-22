/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_file.h
*
* @brief All file related functions pass through here so that user overrides can be used instead
* The functions use void* as the stream as user may want to use a custom file type (not FILE*)
*/

#ifndef _SCE_HEATWAVE_API_FILE_H_
#define _SCE_HEATWAVE_API_FILE_H_

#include "heatwave_api_types.h"

namespace sce{
    namespace HeatWave{

        class FileHandler
        {
        public:
            static void* open(const char* strFilePath);
            static int32_t close(void* pStream);

            static int32_t seek(void *pStream, int32_t uOffset, int32_t nOrigin);
            static int32_t tell(void* pStream);

            static uint32_t read(void* pPtr, uint32_t uSize, uint32_t uCount, void* pStream);

            static SceHwFileFunctions s_userOverrides;
        };

    } /* HeatWave */
} /*sce */

#endif /* _SCE_HEATWAVE_API_FILE_H_ */
