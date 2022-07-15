/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_at9_psvita.h
*
* @brief PSVita implementation of ATRAC9 decoding
*
*/

#ifndef _SCE_HEATWAVE_API_DECODER_AT9_PSVITA_H_
#define _SCE_HEATWAVE_API_DECODER_AT9_PSVITA_H_

#include "heatwave_api_decoder_at9.h"
#include <audiodec.h>

namespace sce{
    namespace HeatWave{

        class DecoderAt9PSVita : public DecoderAt9
        {
        public:
            DecoderAt9PSVita();
            ~DecoderAt9PSVita();

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();
            SceHwError getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset);

        private:
            SceAudiodecCtrl m_ctrl;
            SceAudiodecInfo m_info;

            char* m_pcmBuffer; ///< buffer of decoded pcm data
            char* m_pcmBufferReadPosition; ///< current decoded data read position
            char* m_pcmBufferEnd; ///< end of available decoded pcm data
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_DECODER_AT9_PSVITA_H_ */
