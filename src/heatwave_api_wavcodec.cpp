/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @history
*   1.1     Rename endian functions.
*   1.44    Calculate the file duration, store in AudioCodec::m_nDuration.
*/

#include "heatwave_api_wavcodec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Flip endian if needed (PS3!)
        */
        static uint32_t _flipEndian32(uint32_t uIn)
        {
#if defined __PS3__
            unsigned char   *s1;
            unsigned char   *d1;
            uint32_t        out = 0;

            d1 = (unsigned char *)&out;
            s1 = (unsigned char *)&uIn;

            *d1++ = *(s1 + 3);
            *d1++ = *(s1 + 2);
            *d1++ = *(s1 + 1);
            *d1++ = *s1;

            return (out);
#else
            return uIn;
#endif /* __PS3__ */
        }

        /**
        * @brief Flip endian if needed (PS3!)
        */
        static int16_t _flipEndian16(int16_t nIn)
        {
#if defined __PS3__
            unsigned char   *s1;
            unsigned char   *d1;
            int16_t         out = 0;

            d1 = (unsigned char *)&out;
            s1 = (unsigned char *)&nIn;

            *d1++ = *(s1 + 1);
            *d1++ = *s1;

            return (out);
#else
            return nIn;
#endif /* __PS3__ */
        }

        /**
        * @brief Find the riff chunk in the file
        */
        static bool _scanRiffFileForChunk(const char *strChunkId, sce::HeatWave::DataSource *pStream, int32_t *puChunkSize)
        {
            int32_t  returnCode;
            bool    bFoundChunk = false;
            union
            {
                char      c[4];
                int ui;
            } readBuffer;

            *puChunkSize = 0;

            while (!bFoundChunk)
            {
                returnCode = pStream->seek(*puChunkSize, SEEK_CUR);
                if (returnCode < 0)
                {
                    return false;
                }

                returnCode = pStream->read(readBuffer.c, sizeof(char), 4);
                if (returnCode != 4)
                {
                    return false;
                }

                bFoundChunk = (strncmp(readBuffer.c, strChunkId, 4) == 0);

                returnCode = pStream->read(readBuffer.c, sizeof(char), 4);
                if (returnCode != 4)
                {
                    return false;
                }

                *puChunkSize = _flipEndian32(readBuffer.ui);
            }

            return true;
        }

        /**
        * @brief Check for existence of valid RIFF WAVE header at the start of the data
        */
        bool _sceHeatWaveIsWaveFormat(sce::HeatWave::DataSource *pStream)
        {
            const uint32_t kuRiffChunkSize = 12;
            const uint32_t kuWaveTagOffset = 8;
            const uint32_t kuTagLen = 4;
            char buffer[kuRiffChunkSize];

            pStream->seek(0, SEEK_SET);
            pStream->read(buffer, sizeof(char), kuRiffChunkSize);

            if (strncmp(buffer, "RIFF", kuTagLen) == 0)
            {
                if (strncmp(&buffer[kuWaveTagOffset], "WAVE", kuTagLen) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        WavCodec::WavCodec(DataSource *pSource)
        : AudioCodec(pSource),
        m_dataSectionByteOffset(0),
        m_dataSectionByteSize(0)
        {
        }

        /**
        * @brief Extract information about the audio data and setup for reading from the 'data' section
        */
        SceHwError WavCodec::init()
        {
            char readbuffer[8];
            int32_t chunkSize;
            const float kfSecsToMs = 1000.0f;

            m_dataSource->seek(0, SEEK_SET);

            if (!_sceHeatWaveIsWaveFormat(m_dataSource))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            /* FORMAT */
            if (!_scanRiffFileForChunk("fmt ", m_dataSource, &chunkSize))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            if (m_dataSource->read(readbuffer, sizeof(char), 8) != 8)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* Don't support any of the compressed formats*/
            if (1 != _flipEndian16(*((int16_t*)(readbuffer))))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            m_numberOfChannels = _flipEndian16(*((int16_t*)(void *)(readbuffer + 2)));
            if ((m_numberOfChannels != 0x01) && (m_numberOfChannels != 0x02))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            m_sampleRate = (int)_flipEndian32(*((int*)(void *)(readbuffer + 4)));

            if (m_dataSource->read(readbuffer, sizeof(char), 8) != 8) /* block alignment */
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* Only support 16 bit for now */
            if (16 != _flipEndian16(*((int16_t*)(void *)(readbuffer + 6))))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            /* DATA */
            if (m_dataSource->seek(12, SEEK_SET) < 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (!_scanRiffFileForChunk("data", m_dataSource, &chunkSize))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            m_dataSectionByteOffset = m_dataSource->tell();
            m_dataSectionByteSize = chunkSize;

            m_nDuration = (int32_t)(kfSecsToMs * ((float)m_dataSectionByteSize / ((float)sizeof(int16_t) * (float)m_numberOfChannels * (float)m_sampleRate)));

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Offset into the audio data
        *
        * @param[in] nSeekOffset The milli-second time to offset the read position.
        */
        SceHwError WavCodec::performSeek(int32_t nSeekOffset)
        {
            const float kfSecsToMs = 1000.0f;

            // convert nSeekOffset from milliseconds to a byte offset.
            const uint32_t kuByteOffset = (uint32_t)(nSeekOffset * ((float)getSampleRate()/kfSecsToMs) * getNumberOfChannels() * sizeof(int16_t));
            const uint32_t kuFilepos = m_dataSectionByteOffset + kuByteOffset; // Offset within the data section.

            if (kuByteOffset > m_dataSectionByteSize)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_dataSource->seek(kuFilepos, SEEK_SET);

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read uSizeBytes of audio data as 16bit PCM data
        */
        SceHwError WavCodec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
        {
            const uint32_t currentDataPos = m_dataSource->tell() - m_dataSectionByteOffset;
            uint32_t numBytesToRead = uSizeBytes;

            if (currentDataPos + uSizeBytes > m_dataSectionByteSize) /* clamp read to end of the data section */
            {
                numBytesToRead = m_dataSectionByteSize - currentDataPos;
            }

            *pNumBytesReadOut = m_dataSource->read(pBuffer, 1, numBytesToRead);
            if (*pNumBytesReadOut < numBytesToRead)
            {
                return SCE_HEATWAVE_API_ERROR_FILE;
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
