/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "MP3\heatwave_api_mp3packetheader.h"

namespace sce{
    namespace HeatWave{

        static const uint32_t kuNumVersions = 4; /* 2.5, reserved, 2, 1 */
        static const uint32_t kuNumLayers = 4; /* reserved, 1, 2, 3 */
        static const uint32_t kuNumBitratesPerLayer = 16;
        static const uint32_t kuNumSampleRates = kuNumLayers;
        static const uint32_t kuVersionReserved = 1;
        static const uint32_t kuLayerReserved = 0;
        static const uint32_t kuBitrateReserved = 0;
        static const uint32_t kuBitrateBad = kuNumBitratesPerLayer - 1;

        static const uint16_t samplesPerPacketTable[kuNumVersions][kuNumLayers] = {
            { 0, 576, 1152, 384 }, /* V2.5 */
            { 0, 0, 0, 0 }, /* reserved */
            { 0, 576, 1152, 384 }, /* V2 */
            { 0, 1152, 1152, 384 } /* V1 */
        };

        static const int32_t bitrateTable[kuNumVersions][kuNumLayers][kuNumBitratesPerLayer] = {
            { /* V2.5 */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Layer Reserved */
                { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, /* version 2.5, layer III */
                { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, /* version 2.5, layer I */
                { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }  /* version 2.5, layer I */
            },
            { /* Reserved */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Invalid */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Invalid */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Invalid */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  /* Invalid */
            },
            { /* V2 */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Layer Reserved */
                { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1 }, /* version 2, layer III */
                { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1 }, /* version 2, layer II */
                { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1 } /* version 2, layer I*/
            },
            { /* V1 */
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Layer Reserved */
                { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 }, /* version 1, layer III*/
                { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1 }, /* version 1, layer II */
                { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 }, /* version 1, layer I*/
            },
        };

        static const int32_t knSampleRateTable[kuNumVersions][kuNumSampleRates] = {
            { 11025, 12000, 8000, 0 }, /* V2.5 */
            { 0, 0, 0, 0 }, /* reserved */
            { 22050, 24000, 16000, 0 }, /* V2*/
            { 44100, 48000, 32000, 0 }, /* V1 */
        };

        static const uint32_t kuSlotSizeTable[4] = { 0, 1, 1, 4 };
        static const uint32_t kuChannelsTable[4] = { 2, 2, 2, 1 };


        Mp3PacketHeader::Mp3PacketHeader()
            : syncWord(0),
            version(0),
            layer(0),
            protectionBit(0),
            bitRateIndex(0),
            samplingFrequencyIndex(0),
            paddingBit(0),
            privateBit(0),
            chMode(0),
            modeExtension(0),
            copyrightBit(0),
            originalBit(0),
            emphasis(0),
            bitRate(0),
            samplingFrequency(0),
            numChannels(0)
        {
        }

        /**
        * @brief Extract data from the packet header
        */
        SceHwError Mp3PacketHeader::parse(unsigned char cBuffer[4])
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            syncWord                = (cBuffer[0] & 0xFF) << 4 | (cBuffer[1] & 0xE0) >> 4;
            version                 = (cBuffer[1] & 0x18) >> 3;
            layer                   = (cBuffer[1] & 0x06) >> 1;
            protectionBit           = (cBuffer[1] & 0x01) >> 0;
            bitRateIndex            = (cBuffer[2] & 0xF0) >> 4;
            samplingFrequencyIndex  = (cBuffer[2] & 0x06) >> 2;
            paddingBit              = (cBuffer[2] & 0x02) >> 1;
            privateBit              = (cBuffer[2] & 0x01) >> 0;
            chMode                  = (cBuffer[3] & 0xC0) >> 6;
            modeExtension           = (cBuffer[3] & 0x30) >> 4;
            copyrightBit            = (cBuffer[3] & 0x08) >> 3;
            originalBit             = (cBuffer[3] & 0x04) >> 2;
            emphasis                = (cBuffer[3] & 0x03) >> 0;

            res = isValid();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            bitRate = bitrateTable[version][layer][bitRateIndex];
            samplingFrequency = knSampleRateTable[version][samplingFrequencyIndex];
            numChannels = kuChannelsTable[chMode];

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate the full size of this packet (header and compressed data that follows it)
        */
        SceHwError Mp3PacketHeader::getPacketSizeBytes(uint32_t *pSizeOut) const
        {
            const uint32_t kuKiloBitToBytes = 1000;
            SceHwError res = SCE_HEATWAVE_API_OK;
            uint32_t padding = 0;
            uint32_t bitsPerSample = 0;
            uint32_t packetSizeBytes = 0;

            if (pSizeOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = isValid();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            padding = (paddingBit == 1) ? kuSlotSizeTable[layer] : 0;
            bitsPerSample = samplesPerPacketTable[version][layer] / 8;

            packetSizeBytes = ((bitsPerSample * bitRate * kuKiloBitToBytes) / knSampleRateTable[version][samplingFrequencyIndex]) + padding;
            if (packetSizeBytes == 0)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            *pSizeOut = packetSizeBytes;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief The maximum number of samples per channel of audio data encoded in this MP3 packet
        */
        SceHwError Mp3PacketHeader::getPacketSizeFrames(uint32_t *pNumFramesOut) const
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pNumFramesOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = isValid();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            *pNumFramesOut = samplesPerPacketTable[version][layer];
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Check that values from the header are valid and within correct range
        */
        SceHwError Mp3PacketHeader::isValid() const
        {
            if (syncWord != 0xFFE)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            /* Do not support the reserved values */
            if (version == kuVersionReserved || layer == kuLayerReserved || bitRateIndex == kuBitrateReserved || bitRateIndex == kuBitrateBad)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            /* Range check */
            if (version >= kuNumVersions || layer >= kuNumLayers || bitRateIndex >= kuNumBitratesPerLayer || samplingFrequencyIndex >= kuNumSampleRates)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            return SCE_HEATWAVE_API_OK;
        }

        void Mp3PacketHeader::reset()
        {
            syncWord = 0;
            version = 0;
            layer = 0;
            protectionBit = 0;
            bitRateIndex = 0;
            samplingFrequencyIndex = 0;
            paddingBit = 0;
            privateBit = 0;
            chMode = 0;
            modeExtension = 0;
            copyrightBit = 0;
            originalBit = 0;
            emphasis = 0;
            bitRate = 0;
            samplingFrequency = 0;
            numChannels = 0;
        }

    } /* HeatWave */
} /* sce */
