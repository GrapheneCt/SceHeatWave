/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3_psvita.h
*
* @brief PSVita implementation of MP3 decoding
*
* @history
*   1.1 File added
*/

#ifndef _SCE_HEATWAVE_API_DECODER_MP3_PSVITA_H_
#define _SCE_HEATWAVE_API_DECODER_MP3_PSVITA_H_

#include "heatwave_api_decoder_mp3.h"
#include <audiodec.h>

namespace sce{
    namespace HeatWave{

        class DecoderMp3PSVita : public DecoderMp3
        {
        public:
            DecoderMp3PSVita();
            ~DecoderMp3PSVita();

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();

        private:
            SceAudiodecCtrl m_ctrl;
            SceAudiodecInfo m_info;

            char* m_pcmBuffer; ///< buffer of decoded pcm data
            char* m_pcmBufferReadPosition; ///< current decoded data read position
            char* m_pcmBufferEnd; ///< end of available decoded pcm data
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_DECODER_MP3_PSVITA_H_ */
