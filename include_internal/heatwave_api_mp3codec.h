/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_mp3codec.h
*
* @brief Wrapper for a stream of MP3 format audio data
*
* @history
*   1.1     File added.
*   1.44    Calculate the file duration, store in AudioCodec::m_nDuration.
*           Add support to seek.
*/

#ifndef _SCE_HEATWAVE_API_MP3_CODEC_H_
#define _SCE_HEATWAVE_API_MP3_CODEC_H_

#include "heatwave_api_audiocodec.h"
#include "heatwave_api_ringbuffer.h"
#include "MP3/heatwave_api_mp3packetheader.h"
#include <vector>

namespace sce{
    namespace HeatWave{

        class DecoderMp3;
        class Mp3PacketHeader;

        class Mp3Codec : public AudioCodec
        {
        public:
            static const uint32_t kuId3HeaderSize = 10;

            Mp3Codec(DataSource *pSource);
            virtual ~Mp3Codec();

            static SceHwError isMp3Format(DataSource *pStream);
            static SceHwError getFrameSyncPosition(unsigned char *pMemory, uint32_t uMemSize, uint32_t *pByteOffsetOut);
            static uint32_t getId3Size(unsigned char cBuffer[Mp3Codec::kuId3HeaderSize]);
            static bool isId3Tag(unsigned char cBuffer[3]);

        protected:
            static const uint32_t kuDataBlockSize = 4 * 1024;

            virtual SceHwError init();

            virtual SceHwError performSeek(int32_t nSeekOffset);
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut);

        private:
            SceHwError passBitStreamBlockToDecoder();

            SceHwError createDecoder();

            SceHwError parseMp3Header(BitStream *pBuffer, Mp3PacketHeader *pHeader);

            int32_t readVBRIDuration(const char* pBuffer, const int32_t nSizeBytes);
            int32_t readXingDuration(const char* pBuffer, const int32_t nSizeBytes);

            int32_t checkForVbri(const char* pBuffer, const int32_t nSizeBytes);
            bool readVbriSeekTable(const char* pBuffer, const int32_t nSizeBytes);

            int32_t findXingPos(const char* pBuffer, const int32_t nSizeBytes);
            bool readXingSeekTable(const char* pBuffer, const int32_t nSizeBytes);

            Mp3PacketHeader m_header;
            DecoderMp3* m_decoder;
            BitStream m_buffer;
            RingBuffer* m_ringBuffer;

            int32_t m_nSkipSamples;

            int32_t m_nSamplesPerPacket;

            /**
            * @brief Link audio sample offset with a corresponding byte offset.
            */
            typedef struct SeekEntry
            {
                SeekEntry(int32_t bytePosition, int32_t samplePosition)
                    : nByteOffset(bytePosition),
                    nSampleOffset(samplePosition)
                {}

                int32_t nByteOffset;    // File location in bytes.
                int32_t nSampleOffset;  // Corresponding sample offset.
            } SeekEntry;

            std::vector<SeekEntry> m_seekTable;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_MP3_CODEC_H_ */
