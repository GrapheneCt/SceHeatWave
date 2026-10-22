/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_AT3

#include "heatwave_api_at3codec.h"

#include "heatwave_api.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"
#include "heatwave_api_ringbuffer.h"
#include "ATRAC3/heatwave_api_decoder_at3.h"

#define WAVE_FORMAT_EXTENSIBLE (0xFFFFFFFE)

namespace sce{
    namespace HeatWave{

        SceHwError At3Codec::isAt3WaveFormat(sce::HeatWave::DataSource *pStream)
        {
            return parseAt3RIFF(pStream);
        }

        SceHwError At3Codec::isAt3OmaFormat(sce::HeatWave::DataSource *pStream)
        {
            return parseAt3OMA(pStream);
        }

        bool At3Codec::isId3Tag(unsigned char cBuffer[3])
        {
            return (cBuffer[0] == 'I' && cBuffer[1] == 'D' && cBuffer[2] == '3') ||
                    (cBuffer[0] == 'e' && cBuffer[1] == 'a' && cBuffer[2] == '3');
        }

        uint32_t At3Codec::getId3Size(unsigned char cBuffer[kuId3HeaderSize])
        {
            uint32_t tmp = 0;
            uint32_t size = 0;

            if (!isId3Tag(cBuffer))
            {
                return 0;
            }

            /* Calculate the size of the ID3 section */
            for (int32_t i = 0; i < 4; i++)
            {
                tmp = (cBuffer[i + 6] & 0x7f);
                tmp <<= (7 * (3 - i));
                size |= tmp;
            }

            return size + kuId3HeaderSize;
        }

        /**
        * @brief Check for existence of valid OMA header and parse some params
        */
        SceHwError At3Codec::parseAt3OMA(DataSource *pStream, DecoderAt3::DecoderAt3CodecType *pCodecType, uint8_t *pParam1, uint8_t *pParam2, uint32_t *pDataOffset, uint32_t *pFrameSize)
        {
            unsigned char* buffer = NULL;
            uint32_t id3Size = 0;

            if (pStream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            buffer = (unsigned char*)MemoryHandler::mallocCb(sizeof(unsigned char) * kuDataBlockSize);
            if (buffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            /* Get some data from the stream */
            pStream->seek(0, SEEK_SET);
            pStream->read((void*)buffer, sizeof(unsigned char), kuDataBlockSize);

            /* Skip ID3 at start of the data */
            if (isId3Tag(buffer))
            {
                id3Size = getId3Size(buffer);

                pStream->seek(id3Size, SEEK_SET);
                pStream->read((void*)buffer, sizeof(unsigned char), kuDataBlockSizeMinimal);
            }

            /* Locate EA3 header */
            uint32_t index = 0;
            unsigned char *ea3Header = (unsigned char*)buffer;
            while (*ea3Header == '\0' && index <= kuDataBlockSizeMinimal - 3)
            {
                index++;
                ea3Header++;
            }

            if (ea3Header[0] != 'E' ||
                ea3Header[1] != 'A' ||
                ea3Header[2] != '3')
            {
                MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            DecoderAt3::DecoderAt3CodecType codecType = (DecoderAt3::DecoderAt3CodecType)ea3Header[32];
            uint8_t param1 = 0;
            uint8_t param2 = 0;

            uint32_t frameSize = 0;
            uint32_t codecParams =
                ((uint32_t)ea3Header[33] << 16) |
                ((uint32_t)ea3Header[34] << 8) |
                (uint32_t)ea3Header[35];
            if (codecType == DecoderAt3::eType_AT3P)
            {
                param1 = ea3Header[34];
                param2 = ea3Header[35];
                frameSize = ((codecParams & 0x3FF) * 8) + 8;
            }
            else if (codecType == DecoderAt3::eType_AT3)
            {
                /* Everything here is hardcoded: no ATRAC3 header documentation anywhere */
                param1 = ea3Header[33];
                if (param1 == 2)
                {
                    param1 = 1;
                }

                param2 = ea3Header[35];
                switch (param2)
                {
                case 0x18:
                    param2 = 0x60;
                    break;
                case 0x26:
                    param2 = 0x98;
                    break;
                case 0x30:
                    param2 = 0xC0;
                    break;
                default:
                    MemoryHandler::freeCb(buffer);
                    return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                }

                frameSize = (codecParams & 0x3FF) * 8;
            }
            else
            {
                MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            MemoryHandler::freeCb(buffer);

            if (pCodecType)
            {
                *pCodecType = codecType;
            }

            if (pParam1)
            {
                *pParam1 = param1;
            }

            if (pParam2)
            {
                *pParam2 = param2;
            }

            if (pDataOffset)
            {
                *pDataOffset = id3Size + index + kuEa3HeaderSize;
            }

            if (pFrameSize)
            {
                *pFrameSize = frameSize;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Check for existence of valid ATRAC3 RIFF data and parse some params
        */
        SceHwError At3Codec::parseAt3RIFF(DataSource *pStream, DecoderAt3::DecoderAt3CodecType *pCodecType, uint8_t *pParam1, uint8_t *pParam2, uint32_t *pDataOffset, uint32_t *pSampleCount)
        {
            unsigned char* buffer = NULL;

            if (pStream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            buffer = (unsigned char*)MemoryHandler::mallocCb(sizeof(unsigned char) * kuDataBlockSizeMinimal);
            if (buffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            /* Get some data from the stream */
            pStream->seek(0, SEEK_SET);
            pStream->read((void*)buffer, sizeof(unsigned char), kuDataBlockSizeMinimal);

            uint32_t index = 0;
            bool foundData = false;
            while (index <= kuDataBlockSizeMinimal - 8)
            {
                if (buffer[index] == 'f' &&
                    buffer[index + 1] == 'a' &&
                    buffer[index + 2] == 'c' &&
                    buffer[index + 3] == 't')
                {
                    foundData = true;
                    break;
                }
                index++;
            }

            uint32_t sampleCount = 0;
            if (foundData && index <= kuDataBlockSizeMinimal - 12)
            {
                sampleCount =
                    (uint32_t)buffer[index + 8] |
                    ((uint32_t)buffer[index + 9] << 8) |
                    ((uint32_t)buffer[index + 10] << 16) |
                    ((uint32_t)buffer[index + 11] << 24);
            }

            /* Locate AT3 info header. "data" */
            index = 0;
            foundData = false;
            while (index <= kuDataBlockSizeMinimal - 8)
            {
                if (buffer[index] == 'd' &&
                    buffer[index + 1] == 'a' &&
                    buffer[index + 2] == 't' &&
                    buffer[index + 3] == 'a')
                {
                    foundData = true;
                    break;
                }
                index++;
            }

            if (!foundData)
            {
                MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            index += 8;

            DecoderAt3::DecoderAt3CodecType codecType;
            uint8_t param1 = 0;
            uint8_t param2 = 0;

            uint16_t formatTag = (uint16_t)buffer[20] | ((uint16_t)buffer[21] << 8);
            switch (formatTag)
            {
            case 0xFFFE:
                codecType = DecoderAt3::eType_AT3P;
                break;
            case 0x0270:
                codecType = DecoderAt3::eType_AT3;
                break;
            default:
                MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            if (codecType == DecoderAt3::eType_AT3P)
            {
                param1 = buffer[62];
                param2 = buffer[63];
            }
            else if (codecType == DecoderAt3::eType_AT3)
            {
                /* Everything here is hardcoded: no ATRAC3 header documentation anywhere */
                uint16_t codecInfo = (uint16_t)buffer[28] | ((uint16_t)buffer[29] << 8);
                switch (codecInfo) {
                case 0x204C:
                    param2 = 0x60;
                    param1 = 1;
                    break;
                case 0x3324:
                    param2 = 0x98;
                    param1 = 0;
                    break;
                case 0x4099:
                    param2 = 0xC0;
                    param1 = 0;
                    break;
                default:
                    MemoryHandler::freeCb(buffer);
                    return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                }
            }
            else
            {
                MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            MemoryHandler::freeCb(buffer);

            if (pCodecType)
            {
                *pCodecType = codecType;
            }

            if (pParam1)
            {
                *pParam1 = param1;
            }

            if (pParam2)
            {
                *pParam2 = param2;
            }

            if (pDataOffset)
            {
                *pDataOffset = index;
            }

            if (pSampleCount)
            {
                *pSampleCount = sampleCount;
            }

            return SCE_HEATWAVE_API_OK;
        }

        At3Codec::At3Codec(DataSource *pSource)
        : AudioCodec(pSource),
        m_dataSectionByteOffset(0),
        m_dataSectionByteSize(0),
        m_decoder(NULL),
        m_ringBuffer(NULL)
        {
        }

        At3Codec::~At3Codec()
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
        SceHwError At3Codec::init()
        {
            const float kfSecsToMs = 1000.0f;
            const uint32_t kuAt3PEsGrain = 2048;
            const uint32_t kuAt3EsGrain = 1024;

            uint32_t startOffset;
            uint32_t sampleCount = 0;
            uint32_t frameSize = 0;

            m_dataSource->seek(0, SEEK_END);
            uint32_t dataSize = m_dataSource->tell();

            if (SCE_HEATWAVE_API_FAILED(parseAt3OMA(m_dataSource, &m_codecType, &m_param1, &m_param2, &startOffset, &frameSize)))
            {
                if (SCE_HEATWAVE_API_FAILED(parseAt3RIFF(m_dataSource, &m_codecType, &m_param1, &m_param2, &startOffset, &sampleCount)))
                {
                    return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                }
            }

            m_numberOfChannels = kuChannels;
            m_sampleRate = kuSampleRate;

            m_dataSectionByteOffset = startOffset;
            m_dataSectionByteSize = dataSize - startOffset;

            if (sampleCount == 0)
            {
                if (frameSize != 0)
                {
                    uint32_t totalEsSamples = m_dataSectionByteSize / frameSize;
                    switch (m_codecType)
                    {
                    case DecoderAt3::eType_AT3P:
                        sampleCount = totalEsSamples * kuAt3PEsGrain;
                        break;
                    case DecoderAt3::eType_AT3:
                        sampleCount = totalEsSamples * kuAt3EsGrain;
                        break;
                    }
                }
            }

            if (sampleCount != 0)
            {
                m_nDuration = (int32_t)((float)sampleCount / (float)m_sampleRate * kfSecsToMs);
            }
            else
            {
                m_nDuration = knDurationUnknown;
            }

            if (!DecoderAt3::isAvailable(m_numberOfChannels))
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Offset into the audio data
        *
        * @param[in] nSeekOffset The milli-second time to offset the read position.
        */
        SceHwError At3Codec::performSeek(int32_t nSeekOffset)
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
        SceHwError At3Codec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
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

                if (numBytesDecoded < numBytesRequested && m_decoder->getType() == DecoderAt3::ePushPull) /* Give the decoder more data */
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
        * @brief Pass a block of the ATRAC3 data to the decoder
        */
        SceHwError At3Codec::passBitStreamBlockToDecoder()
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

        SceHwError At3Codec::createDecoder()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            /* Setup the platform specific decoder */
            m_decoder = DecoderAt3::createAt3DecoderInstance();
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

            res = m_decoder->init(m_dataSource, m_codecType, m_param1, m_param2);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_decoder->getType() == DecoderAt3::ePull)
            {
                MemoryHandler::freeCb(m_buffer.pData); /* decoder will be handling access of the data */
                m_buffer.pData = NULL;
                m_buffer.uSizeBytes = 0;
            }
            else if (m_decoder->getType() == DecoderAt3::ePushPull)
            {
                m_ringBuffer = new RingBuffer(kuDataBlockSize); /* so we can read in cache friendly blocks from the data source */
            }

            return res;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT9 */
