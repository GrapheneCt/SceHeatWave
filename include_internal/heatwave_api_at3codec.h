/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_at3codec.h
*
* @brief Wrapper for ATRAC3 format audio data
*
*/

#ifndef _SCE_HEATWAVE_API_AT3_CODEC_H_
#define _SCE_HEATWAVE_API_AT3_CODEC_H_

#if defined SCE_HEATWAVE_CODEC_AT3

#include "heatwave_api_audiocodec.h"
#include "heatwave_api_ringbuffer.h"
#include "ATRAC3/heatwave_api_decoder_at3.h"

namespace sce{
    namespace HeatWave{

        class At3Codec : public AudioCodec
        {
        public:

            static const uint32_t kuId3HeaderSize = 10;
			static const uint32_t kuEa3HeaderSize = 96;

            At3Codec(DataSource *pSource);
            virtual ~At3Codec();

			static SceHwError isAt3WaveFormat(sce::HeatWave::DataSource *pStream);
			static SceHwError isAt3OmaFormat(sce::HeatWave::DataSource *pStream);
			static SceHwError parseAt3OMA(DataSource *pStream, DecoderAt3::DecoderAt3CodecType *pCodecType = NULL, uint8_t *pParam1 = NULL, uint8_t *pParam2 = NULL, uint32_t *pDataOffset = NULL, uint32_t *pFrameSize = NULL);
            static SceHwError parseAt3RIFF(DataSource *pStream, DecoderAt3::DecoderAt3CodecType *pCodecType = NULL, uint8_t *pParam1 = NULL, uint8_t *pParam2 = NULL, uint32_t *pDataOffset = NULL, uint32_t *pSampleCount = NULL);
            static uint32_t getId3Size(unsigned char cBuffer[At3Codec::kuId3HeaderSize]);
            static bool isId3Tag(unsigned char cBuffer[3]);

        protected:
            static const uint32_t kuDataBlockSize = 4 * 1024;
			static const uint32_t kuDataBlockSizeMinimal = 1024;

			static const uint32_t kuChannels = 2;
			static const uint32_t kuSampleRate = 44100;

            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uNumFrames, uint32_t *pNumFramesReadOut);

        private:
            SceHwError passBitStreamBlockToDecoder();

            SceHwError createDecoder();

			uint8_t m_param1;
			uint8_t m_param2;
			DecoderAt3::DecoderAt3CodecType m_codecType;

            uint32_t m_dataSectionByteOffset;
            uint32_t m_dataSectionByteSize;

            DecoderAt3* m_decoder;
            BitStream m_buffer;
            RingBuffer* m_ringBuffer;
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT3 */

#endif /* _SCE_HEATWAVE_API_AT3_CODEC_H_ */
