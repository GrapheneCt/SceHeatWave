/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_at3_psvita.h
*
* @brief PSVita implementation of ATRAC3 decoding
*
*/

#ifndef _SCE_HEATWAVE_API_DECODER_AT3_PSVITA_H_
#define _SCE_HEATWAVE_API_DECODER_AT3_PSVITA_H_

#if defined SCE_HEATWAVE_CODEC_AT3

#include "heatwave_api_decoder_at3.h"
#include <audiocodec.h>
#include <libsysmodule.h>

namespace sce{
    namespace HeatWave{

        class DecoderAt3PSVita : public DecoderAt3
        {
        public:
            DecoderAt3PSVita();
            ~DecoderAt3PSVita();

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();
            SceHwError getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset);

        private:
            static const uint32_t kuSamplingRate = 44100;

            int32_t getAt3ConfigPSP2(uint32_t cmode, uint32_t nbytes);

            SceAudiocodecCtrl m_ctrl;
            uint32_t m_sceCodecType;
            uint32_t m_pcmBlockSize;

            char* m_pcmBuffer; ///< buffer of decoded pcm data
            char* m_pcmBufferReadPosition; ///< current decoded data read position
            char* m_pcmBufferEnd; ///< end of available decoded pcm data
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT3 */

#endif /* _SCE_HEATWAVE_API_DECODER_AT3_PSVITA_H_ */
