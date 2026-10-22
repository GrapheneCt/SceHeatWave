/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_oggcodec.h
*
* @brief Wrapper for a stream of OGG Vorbis format audio data
*
*/

#ifndef _SCE_HEATWAVE_API_OGG_CODEC_H_
#define _SCE_HEATWAVE_API_OGG_CODEC_H_

#if defined SCE_HEATWAVE_CODEC_OGG

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "heatwave_api_audiocodec.h"

namespace sce{
    namespace HeatWave{

        class OggCodec : public AudioCodec
        {
        public:

            OggCodec(DataSource *pSource);
            virtual ~OggCodec();

            static SceHwError isOggFormat(DataSource *pStream);

        protected:
            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut);

        private:

            static uint32_t OggRead(void *ptr, uint32_t size, uint32_t nmemb, void *datasource);
            static int32_t OggSeek(void *datasource, ogg_int64_t offset, int32_t whence);
            static long OggTell(void *datasource);
            static int32_t OggClose(void *datasource);

            OggVorbis_File m_ogg;
            int32_t m_decoderExists;
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_OGG */

#endif /* _SCE_HEATWAVE_API_OGG_CODEC_H_ */
