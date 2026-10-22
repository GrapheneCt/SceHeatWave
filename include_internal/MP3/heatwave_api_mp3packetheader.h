/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_mp3packetheader.h
*
* @brief Wrapper for MP3 packet header
*
* @history
*   1.1 File added
*/

#ifndef _SCE_HEATWAVE_API_MP3_PACKET_HEADER_H_
#define _SCE_HEATWAVE_API_MP3_PACKET_HEADER_H_

#if defined SCE_HEATWAVE_CODEC_MP3

#include "heatwave_api_errors.h"
#include "heatwave_api_types.h"
#include "..\heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class Mp3PacketHeader : public MemoryHandler
        {
        public:
            static const uint32_t kuFrameHeaderSizeBytes = 4;

            uint32_t syncWord;
            uint32_t version;
            uint32_t layer;
            uint32_t protectionBit;
            uint32_t bitRateIndex;
            uint32_t samplingFrequencyIndex;
            uint32_t paddingBit;
            uint32_t privateBit;
            uint32_t chMode;
            uint32_t modeExtension;
            uint32_t copyrightBit;
            uint32_t originalBit;
            uint32_t emphasis;
            uint32_t bitRate;
            uint32_t samplingFrequency;
            uint32_t numChannels;

            Mp3PacketHeader();

            SceHwError parse(unsigned char cBuffer[4]);
            SceHwError getPacketSizeBytes(uint32_t *pSizeOut) const;
            SceHwError getPacketSizeFrames(uint32_t *pNumFramesOut) const;
            SceHwError isValid() const;

            void reset();
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */

#endif /* _SCE_HEATWAVE_API_MP3_PACKET_HEADER_H_ */
