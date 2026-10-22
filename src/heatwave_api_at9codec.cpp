/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_AT9

#include "heatwave_api_at9codec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"
#include "heatwave_api_ringbuffer.h"
#include "ATRAC9/heatwave_api_decoder_at9.h"

#define WAVE_FORMAT_EXTENSIBLE (0xFFFFFFFE)

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
        * @brief Check for existence of valid RIFF WAVE header and ATRAC9 format at the start of the data
        */
        bool _sceHeatWaveIsAt9Format(sce::HeatWave::DataSource *pStream)
        {
            const uint32_t kuFmtChunkSize = 52;
            const uint32_t kuRiffChunkSize = 12;
            const uint32_t kuWaveTagOffset = 8;
            const uint32_t kuTagLen = 4;

            const uint8_t kuSubFormat[16] = {
                // 0x47E142D2, 0x36BA, 0x4D8D, 0x88FC61654F8C836C
                0xD2, 0x42, 0xE1, 0x47, 0xBA, 0x36, 0x8D, 0x4D, 0x88, 0xFC, 0x61, 0x65, 0x4F, 0x8C, 0x83, 0x6C
            };

            char buffer[kuFmtChunkSize];

            pStream->seek(kuWaveTagOffset + kuTagLen, SEEK_SET);
            pStream->read(buffer, sizeof(char), kuFmtChunkSize);

            if (strncmp(buffer, "fmt ", kuTagLen) == 0)
            {
                if (_flipEndian16(*((int16_t*)(void *)(buffer + kuTagLen + 4))) == WAVE_FORMAT_EXTENSIBLE && !memcmp(kuSubFormat, buffer + 32, sizeof(kuSubFormat)))
                {
                    pStream->seek(kuRiffChunkSize, SEEK_SET);
                    return true;
                }
            }

            return false;
        }

        At9Codec::At9Codec(DataSource *pSource)
        : AudioCodec(pSource),
        m_dataSectionByteOffset(0),
        m_dataSectionByteSize(0),
        m_decoder(NULL),
        m_ringBuffer(NULL),
        m_configData(0)
        {
        }

        At9Codec::~At9Codec()
        {
            if (m_decoder != NULL)
            {
                delete m_decoder;
                m_decoder = NULL;
            }

            if (m_buffer.pData != NULL)
            {
                MemoryHandler::freeCb(m_buffer.pData);
                m_buffer.pData = NULL;
            }

            if (m_ringBuffer != NULL)
            {
                delete m_ringBuffer;
                m_ringBuffer = NULL;
            }
        }

        /**
        * @brief Extract information about the audio data and setup for reading from the 'data' section
        */
        SceHwError At9Codec::init()
        {
            char readbuffer[32];
            int32_t chunkSize;
            uint32_t totalSamples = 0;
            const float kfSecsToMs = 1000.0f;

            m_dataSource->seek(0, SEEK_SET);

            if (!_sceHeatWaveIsAt9Format(m_dataSource))
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

            m_numberOfChannels = _flipEndian16(*((int16_t*)(void *)(readbuffer + 2)));
            if ((m_numberOfChannels != 0x01) && (m_numberOfChannels != 0x02))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            m_sampleRate = (int)_flipEndian32(*((int*)(void *)(readbuffer + 4)));

            if (m_dataSource->read(readbuffer, sizeof(char), 32) != 32) /* skip */
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (m_dataSource->read(readbuffer, sizeof(char), 8) != 8)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_configData = *(int32_t*)(void *)(readbuffer + 4);

            /* FACT */
            if (m_dataSource->seek(12, SEEK_SET) < 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (!_scanRiffFileForChunk("fact", m_dataSource, &chunkSize))
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            if (m_dataSource->read(readbuffer, sizeof(char), 8) != 8)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            totalSamples = *(uint32_t*)(void *)(readbuffer);

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

            m_nDuration = (int32_t)((float)totalSamples / (float)m_sampleRate * kfSecsToMs);

            if (!DecoderAt9::isAvailable(m_numberOfChannels))
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Offset into the audio data
        *
        * @param[in] nSeekOffset The milli-second time to offset the read position.
        */
        SceHwError At9Codec::performSeek(int32_t nSeekOffset)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            uint32_t uByteOffset = 0;

            if (m_decoder == NULL) {
                res = createDecoder();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            // convert nSeekOffset from milliseconds to a byte offset.
            res = m_decoder->getSeekOffset(nSeekOffset, &uByteOffset);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            const uint32_t kuFilepos = m_dataSectionByteOffset + uByteOffset; // Offset within the data section.

            if (uByteOffset > m_dataSectionByteSize)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_dataSource->seek(kuFilepos, SEEK_SET);

            res = m_decoder->reset();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_ringBuffer != NULL)
            {
                m_ringBuffer->reset();
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read uSizeBytes of audio data as 16bit PCM data
        */
        SceHwError At9Codec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
        {
            uint32_t numBytesRequested = uSizeBytes;
            uint32_t numBytesDecoded = 0;
            SceHwError res = SCE_HEATWAVE_API_OK;
            int16_t *outBuffer = pBuffer;
            uint32_t timeoutcounter = 32;

            if (m_decoder == NULL) {
                res = createDecoder();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            *pNumBytesReadOut = 0;

            while (numBytesRequested > 0)
            {
                res = m_decoder->decode(outBuffer, numBytesRequested, &numBytesDecoded);
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                if (numBytesDecoded < numBytesRequested && m_decoder->getType() == DecoderAt9::ePushPull) /* Give the decoder more data */
                {
                    res = passBitStreamBlockToDecoder();

                    if (SCE_HEATWAVE_API_FAILED(res))
                    {
                        return res;
                    }
                }

                outBuffer += numBytesDecoded / sizeof(int16_t);
                numBytesRequested -= numBytesDecoded;
                *pNumBytesReadOut += numBytesDecoded;

                timeoutcounter--;
                if (timeoutcounter == 0) /* prevent any potential for soft locks due to decoder failures */
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass a block of the ATRAC9 data to the decoder
        */
        SceHwError At9Codec::passBitStreamBlockToDecoder()
        {
            const uint32_t kuReadBlockSize = 1024;
            SceHwError res = SCE_HEATWAVE_API_OK;
            uint32_t bytesUsed = 0;
            BitStream stream1;
            BitStream stream2;
            uint32_t bufferAvailable = 0;

            if (m_ringBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            /* Ensure m_ringBuffer is as full as possible */
            bufferAvailable = m_ringBuffer->availableWrite();
            if (bufferAvailable > kuReadBlockSize) /* Only fill in cache friendly blocks */
            {
                const uint32_t rem = bufferAvailable % kuReadBlockSize;
                uint32_t dataToProcess = bufferAvailable - rem;

                dataToProcess = m_dataSource->read(m_buffer.pData, 1, dataToProcess);

                if (dataToProcess > 0) /* Pass the data into the ring buffer */
                {
                    m_ringBuffer->lockWrite(dataToProcess, &stream1, &stream2);

                    if (stream1.uSizeBytes > 0)
                    {
                        MemoryHandler::memcpyCb(stream1.pData, m_buffer.pData, stream1.uSizeBytes);
                    }

                    if (stream2.uSizeBytes > 0)
                    {
                        MemoryHandler::memcpyCb(stream2.pData, m_buffer.pData + stream1.uSizeBytes, stream2.uSizeBytes);
                    }

                    m_ringBuffer->unlockWrite(dataToProcess);
                }
            }

            bufferAvailable = m_ringBuffer->availableRead();
            if (bufferAvailable == 0) /* Ring buffer has emptied, no more data to process */
            {
                return SCE_HEATWAVE_API_ERROR_FILE;
            }

            /* Access all read data available in the ringbuffer */
            m_ringBuffer->lockRead(bufferAvailable, &stream1, &stream2);

            /* Read data from ringbuffer into a continuous section of memory */
            if (stream1.uSizeBytes > 0)
            {
                MemoryHandler::memcpyCb(m_buffer.pData, stream1.pData, stream1.uSizeBytes);
            }

            if (stream2.uSizeBytes > 0)
            {
                MemoryHandler::memcpyCb(m_buffer.pData + stream1.uSizeBytes, stream2.pData, stream2.uSizeBytes);
            }

            m_buffer.uSizeBytes = stream1.uSizeBytes + stream2.uSizeBytes;

            res = m_decoder->fill(&m_buffer, &bytesUsed);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_ringBuffer->unlockRead(bytesUsed);
            if (bytesUsed > m_buffer.uSizeBytes)
            {
                m_dataSource->seek(bytesUsed - m_buffer.uSizeBytes, SEEK_CUR);
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError At9Codec::createDecoder()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            /* Setup the platform specific decoder */
            m_decoder = DecoderAt9::createAt9DecoderInstance();
            if (m_decoder == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            /* buffer to allow cache friendly reads from m_dataSource */
            m_buffer.pData = (char*)sce::HeatWave::MemoryHandler::mallocCb(kuDataBlockSize);
            if (m_buffer.pData == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }
            m_buffer.uSizeBytes = kuDataBlockSize;

            res = m_decoder->init(m_dataSource, m_numberOfChannels, m_sampleRate, m_configData);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_decoder->getType() == DecoderAt9::ePull)
            {
                MemoryHandler::freeCb(m_buffer.pData); /* decoder will be handling access of the data */
                m_buffer.pData = NULL;
                m_buffer.uSizeBytes = 0;
            }
            else if (m_decoder->getType() == DecoderAt9::ePushPull)
            {
                m_ringBuffer = new RingBuffer(kuDataBlockSize); /* so we can read in cache friendly blocks from the data source */
            }

            return res;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT9 */
