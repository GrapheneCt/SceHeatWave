/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_adpcmcodec.h
*
* @brief Wrapper for ADPCM format audio data
*
*/

#ifndef _SCE_HEATWAVE_API_ADPCM_CODEC_H_
#define _SCE_HEATWAVE_API_ADPCM_CODEC_H_

#if defined SCE_HEATWAVE_CODEC_HEVAG

#include "heatwave_api_audiocodec.h"

namespace sce{
    namespace HeatWave{

        extern bool _sceHeatWaveIsAdpcmFormat(sce::HeatWave::DataSource *pStream);

        class AdpcmCodec : public AudioCodec
        {
        public:
            AdpcmCodec(DataSource *pSource);

        protected:
            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uNumFrames, uint32_t *pNumFramesReadOut);

        private:

            class VAGHeader
            {
            public:
                unsigned char ID[4];
                unsigned int  version;
                unsigned char reserved[4];
                unsigned int  dataSize;
                unsigned int  fs;
                unsigned char reserved2[10];
                unsigned char ch;
                unsigned char reserved3;
                unsigned char name[16];
            };

            uint32_t m_dataSectionByteOffset;
            uint32_t m_dataSectionByteSize;
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_HEVAG */

#endif /* _SCE_HEATWAVE_API_ADPCM_CODEC_H_ */
