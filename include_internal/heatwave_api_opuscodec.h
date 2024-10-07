/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_oggcodec.h
*
* @brief Wrapper for a stream of OGG Vorbis format audio data
*
*/

#ifndef _SCE_HEATWAVE_API_OPUS_CODEC_H_
#define _SCE_HEATWAVE_API_OPUS_CODEC_H_

#if defined SCE_HEATWAVE_CODEC_OPUS

#include <opusfile.h>

#include "heatwave_api_audiocodec.h"

namespace sce{
    namespace HeatWave{

        class OpusCodec : public AudioCodec
        {
        public:

            OpusCodec(DataSource *pSource);
            virtual ~OpusCodec();

            static SceHwError isOpusFormat(DataSource *pStream);

        protected:
            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut);

        private:

            static int32_t OpusRead(void *datasource, unsigned char *ptr, int32_t nbytes);
            static int32_t OpusSeek(void *datasource, opus_int64 offset, int32_t whence);
            static opus_int64 OpusTell(void *datasource);
            static int32_t OpusClose(void *datasource);

            OggOpusFile *m_opus;
            int32_t m_decoderExists;
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_OPUS */

#endif /* _SCE_HEATWAVE_API_OPUS_CODEC_H_ */
