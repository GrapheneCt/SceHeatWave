/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3_orbis.h
*
* @brief Orbis implementation of MP3 decoding
*
* @history
*   1.1 File added
*/

#ifndef _SCE_HEATWAVE_API_DECODER_MP3_ORBIS_H_
#define _SCE_HEATWAVE_API_DECODER_MP3_ORBIS_H_

#if defined SCE_HEATWAVE_CODEC_MP3

#include "heatwave_api_decoder_mp3.h"
#include <audiodec.h>

namespace sce{
    namespace HeatWave{

        class DecoderMp3Orbis : public DecoderMp3
        {
        public:
            DecoderMp3Orbis();
            ~DecoderMp3Orbis();

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();

        private:
            SceAudiodecCtrl m_ctrl; ///< libAudioDec - Control parameter to perform decode
            SceAudiodecParamMp3 m_param; ///< libAudioDec - MP3 specific decode parameters
            SceAudiodecAuInfo m_bst; ///< libAudioDec - information about the source encoded data buffer
            SceAudiodecPcmItem m_pcm; ///< libAudioDec - information about the target decoded buffer
            SceAudiodecMp3Info m_info; ///< libAudioDec - information about the MP3 data
            int32_t m_handle; ///< libAudioDec - handle to the decoder context

            char* m_pcmBuffer; ///< buffer of decoded pcm data
            char* m_pcmBufferReadPosition; ///< current decoded data read position
            char* m_pcmBufferEnd; ///< end of available decoded pcm data
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */

#endif /* _SCE_HEATWAVE_API_DECODER_MP3_ORBIS_H_ */
