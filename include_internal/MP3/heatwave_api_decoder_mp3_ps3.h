/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3_ps3.h
*
* @brief PS3 implementation of MP3 decoding
*
* @history
*   1.1 File added
*/

#ifndef _SCE_HEATWAVE_API_DECODER_MP3_PS3_H_
#define _SCE_HEATWAVE_API_DECODER_MP3_PS3_H_

#if defined SCE_HEATWAVE_CODEC_MP3

#include "heatwave_api_decoder_mp3.h"

#include <cell/codec.h>

namespace sce{
    namespace HeatWave{

        class DecoderMp3PS3 : public DecoderMp3
        {
        public:
            DecoderMp3PS3();
            ~DecoderMp3PS3();

            void SetDecodeCompleteFlag(uint32_t uFlag);

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();

        private:
            CellAdecHandle m_decHandle; ///< Handle to the decoder context
            CellAdecAuInfo m_info; ///< Decoder information
            CellAdecPcmItem g_pcm_item; ///< Decoder pcm output
            CellAdecParamMP3 m_param;  ///< Decoder parameters
            CellAdecResource m_resource; ///< Decoder resource

            sys_mutex_t m_decodeMutex;
            sys_cond_t  m_decodeCond;
            uint32_t m_decodeCompleteFlag;

            char* m_pcmBuffer; ///< buffer of decoded pcm data
            char* m_pcmBufferReadPosition; ///< current decoded data read position
            char* m_pcmBufferEnd; ///< end of available decoded pcm data
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */

#endif /* _SCE_HEATWAVE_API_DECODER_MP3_PS3_H_ */
