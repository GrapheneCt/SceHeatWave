/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "heatwave_api_adpcmcodec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Flip endian if needed
        */
        static uint32_t _flipEndian32(uint32_t uIn)
        {
#if !defined __PS3__
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
        * @brief Check for existence of valid ADPCM header at the start of the data
        */
        bool _sceHeatWaveIsAdpcmFormat(sce::HeatWave::DataSource *pStream)
        {
            const uint32_t kuVagMagic = 0x56414770; /* VAGp */
            uint32_t buffer = 0;

            pStream->seek(0, SEEK_SET);
            pStream->read((char *)&buffer, sizeof(char), 4);

            if (_flipEndian32(buffer) == kuVagMagic)
            {
                pStream->seek(0, SEEK_SET);
                return true;
            }

            return false;
        }

        AdpcmCodec::AdpcmCodec(DataSource *pSource)
        : AudioCodec(pSource),
        m_dataSectionByteOffset(0),
        m_dataSectionByteSize(0)
        {
        }

        /**
        * @brief Extract information about the audio data and setup for reading from the data section
        */
        SceHwError AdpcmCodec::init()
        {
            VAGHeader header;
            const float kfSecsToMs = 1000.0f;

            m_dataSource->seek(0, SEEK_SET);

            if (!_sceHeatWaveIsAdpcmFormat(m_dataSource))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            if (m_dataSource->read((char *)&header, sizeof(char), sizeof(VAGHeader)) != sizeof(VAGHeader))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (header.ch == 0)
                m_numberOfChannels = 1;
            else
                m_numberOfChannels = header.ch;
            m_sampleRate = _flipEndian32(header.fs);

            m_dataSectionByteOffset = m_dataSource->tell();
            m_dataSectionByteSize = _flipEndian32(header.dataSize);
            m_bIsAdpcm = true;

            uint32_t sampleCount = m_dataSectionByteSize / 16 * 28;
            m_nDuration = (int32_t)(kfSecsToMs * ((float)sampleCount * (float)m_numberOfChannels / (float)m_sampleRate / 4.0f));

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Offset into the audio data
        *
        * @param[in] nSeekOffset The milli-second time to offset the read position.
        */
        SceHwError AdpcmCodec::performSeek(int32_t nSeekOffset)
        {
            const float kfSecsToMs = 1000.0f;

            // convert nSeekOffset from milliseconds to a byte offset.
            const uint32_t kuSampleOffset = (uint32_t)(((float)nSeekOffset * (float)getSampleRate() * 4.0f) / ((float)getNumberOfChannels() * kfSecsToMs));
            uint32_t kuByteOffset = (16 * kuSampleOffset) / 28;
            kuByteOffset = ((kuByteOffset)+((16) - 1)) & ~((16) - 1); // Align to 16 (ADPCM block size)
            if ((kuByteOffset / 16) % 2 != 0) // Prevent channel swapping due to block interleaving
                kuByteOffset = kuByteOffset - 16;
            const uint32_t kuFilepos = m_dataSectionByteOffset + kuByteOffset; // Offset within the data section.

            if (kuByteOffset > m_dataSectionByteSize)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_dataSource->seek(kuFilepos, SEEK_SET);

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read uSizeBytes of audio data
        */
        SceHwError AdpcmCodec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
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
