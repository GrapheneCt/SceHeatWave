/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_at9codec.h
*
* @brief Wrapper for ATRAC9 format audio data
*
*/

#ifndef _SCE_HEATWAVE_API_AT9_CODEC_H_
#define _SCE_HEATWAVE_API_AT9_CODEC_H_

#include "heatwave_api_audiocodec.h"
#include "heatwave_api_ringbuffer.h"
#include "ATRAC9/heatwave_api_decoder_at9.h"

namespace sce{
    namespace HeatWave{

        extern bool _sceHeatWaveIsAt9Format(sce::HeatWave::DataSource *pStream);

        class At9Codec : public AudioCodec
        {
        public:
            At9Codec(DataSource *pSource);

        protected:
            static const uint32_t kuDataBlockSize = 4 * 1024;

            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uNumFrames, uint32_t *pNumFramesReadOut);

        private:
            SceHwError passBitStreamBlockToDecoder();

            SceHwError createDecoder();

            uint32_t m_dataSectionByteOffset;
            uint32_t m_dataSectionByteSize;
            int32_t m_configData;

            DecoderAt9* m_decoder;
            BitStream m_buffer;
            RingBuffer* m_ringBuffer;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_AT9_CODEC_H_ */
