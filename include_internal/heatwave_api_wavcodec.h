/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_wavcodec.h
*
* @brief Wrapper for Wav format audio data
*
* @history
*   1.1     File added
*           _sceHeatWaveIsWaveFormat() - do not scan the complete file for "RIFF" and "WAVE" tags
*   1.44    Change ::performSeek() to be a milli-second offset from start of audio data.
*/

#ifndef _SCE_HEATWAVE_API_WAV_CODEC_H_
#define _SCE_HEATWAVE_API_WAV_CODEC_H_

#include "heatwave_api_audiocodec.h"

namespace sce{
    namespace HeatWave{

        extern bool _sceHeatWaveIsWaveFormat(sce::HeatWave::DataSource *pStream);

        class WavCodec : public AudioCodec
        {
        public:
            WavCodec(DataSource *pSource);

        protected:
            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uNumFrames, uint32_t *pNumFramesReadOut);

        private:
            uint32_t m_dataSectionByteOffset;
            uint32_t m_dataSectionByteSize;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_WAV_CODEC_H_ */
