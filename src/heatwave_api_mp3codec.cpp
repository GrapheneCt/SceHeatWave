/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "heatwave_api_mp3codec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"
#include "MP3/heatwave_api_mp3packetheader.h"
#include "MP3/heatwave_api_decoder_mp3.h"
#include "heatwave_api.h"

/**
* @brief Multiplier to covert from seconds to milliseconds.
*
* 1sec == 1000mS
*/
#define SECS_TO_MS (1000.0f)

/**
* @brief Flag to indicate duration tag\information was not found in the MP3 data.
*/
#define NOT_FOUND (-1)

namespace sce{
    namespace HeatWave{

        /**
        * @brief Flip endian from big to the native endian of the platform.
        *
        * For PS3 (big endian) no flip is performed, for all other platforms 
        * the endian is flipped from big to little.
        *
        * @param[in] uIn The big endian format uint32_t
        *
        * @return uIn in the correct endian format for the platform.
        */
        static uint32_t _bigToPlatformEndian32(uint32_t uIn)
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
        * @brief Flip the endian from big to the native endian of the platform.
        *
        * For PS3 (big endian) no flip is performed, for all other platforms
        * the endian is flipped from big to little.
        *
        * @param[in] uIn The big endian format uint16_t
        *
        * @return uIn in the correct endian format for the platform.
        */
        static uint16_t _bigToPlatformEndian16(uint16_t uIn)
        {
#if !defined __PS3__
            unsigned char   *s1;
            unsigned char   *d1;
            int16_t         out = 0;

            d1 = (unsigned char *)&out;
            s1 = (unsigned char *)&uIn;

            *d1++ = *(s1 + 1);
            *d1++ = *s1;

            return (out);
#else
            return uIn;
#endif /* __PS3__ */
        }

        /**
        * @brief Utility to convert from a count of MP3 packets to a milli-second duration.
        *
        * @param[in] nPacketCount       The number of MP3 packets.
        * @param[in] nSamplesPerPacket  The number of audio samples per channel in an MP3 packet.
        * @param[in] nSampleRate        The sample rate of the audio data.
        *
        * @return nPacketCount as a milli-second duration.
        */
        inline static int32_t _packetsToMillisecs(const int32_t nPacketCount, const int32_t nSamplesPerPacket, 
            const int32_t nSampleRate)
        {
            return (int32_t)((float)(nPacketCount * nSamplesPerPacket) / (float)nSampleRate * SECS_TO_MS);
        }

        /**
        * @brief Iterate over each byte in the memory region until mp3 frame sync flag is found (bit pattern 1111 1111 111X XXXX)
        */
        SceHwError Mp3Codec::getFrameSyncPosition(unsigned char *pMemory, uint32_t uMemSize, uint32_t *pByteOffsetOut)
        {
            const uint32_t kuFrameSync1 = 0xFF;
            const uint32_t kuFrameSync2 = 0xE0;
            const uint32_t kuMp3TagByteSize = 128;

            unsigned char* mem = pMemory;
            unsigned char* memEnd = pMemory + uMemSize;

            sce::HeatWave::Mp3PacketHeader frameHeader;
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pMemory == NULL || uMemSize == 0 || pByteOffsetOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            while (mem < memEnd - kuId3HeaderSize)
            {
                if (isId3Tag(mem)) /* Skip ID3v2 tags */
                {
                    mem = mem + getId3Size(mem);
                    if (mem >= memEnd)
                    {
                        *pByteOffsetOut = (uint32_t)(mem - pMemory);
                        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
                    }

                    continue;
                }
                else if (mem[0] == 'T' && mem[1] == 'A' && mem[2] == 'G') /* Skip ID2v1 tag */
                {
                    mem += kuMp3TagByteSize;
                    if (mem >= memEnd)
                    {
                        *pByteOffsetOut = (uint32_t)(mem - pMemory);
                        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
                    }
                }
                else if (((mem[0] & kuFrameSync1) == kuFrameSync1) && ((mem[1] & kuFrameSync2) == kuFrameSync2)) /* Found the sync pattern */
                {
                    res = frameHeader.parse(mem); /* Check that the frame header is actually valid (sync pattern could occur in any data...) */
                    if (SCE_HEATWAVE_API_SUCCEEDED(res))
                    {
                        *pByteOffsetOut = (uint32_t)(mem - pMemory);
                        return SCE_HEATWAVE_API_OK;
                    }
                }
                mem++;
            }

            return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
        }

        bool Mp3Codec::isId3Tag(unsigned char cBuffer[3])
        {
            return cBuffer[0] == 'I' && cBuffer[1] == 'D' && cBuffer[2] == '3';
        }

        uint32_t Mp3Codec::getId3Size(unsigned char cBuffer[kuId3HeaderSize])
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
        * @brief Verify pStream is MP3 format by finding successive valid MP3 frame headers
        */
        SceHwError Mp3Codec::isMp3Format(DataSource *pStream)
        {
            const uint32_t kuNumFramesToCheck = 2; ///< Number of successive valid frames required to pass

            uint32_t framesyncpos = 0;
            unsigned char *bufferPos = NULL;
            uint32_t frameSizeBytes = 0;
            sce::HeatWave::Mp3PacketHeader frameHeader;
            SceHwError res = SCE_HEATWAVE_API_OK;
            unsigned char* buffer = NULL;

            if (pStream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            buffer = (unsigned char*)sce::HeatWave::MemoryHandler::mallocCb(sizeof(unsigned char) * kuDataBlockSize);
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
                const uint32_t id3Size = getId3Size(buffer);

                pStream->seek(id3Size, SEEK_SET);
                pStream->read((void*)buffer, sizeof(unsigned char), kuDataBlockSize);
            }

            /* Find a frame sync */
            res = getFrameSyncPosition(buffer, kuDataBlockSize, &framesyncpos);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                sce::HeatWave::MemoryHandler::freeCb(buffer);
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            bufferPos = buffer + framesyncpos;

            /* Check the frame header is valid, skip to next */
            for (uint32_t i = 0; i < kuNumFramesToCheck; i++)
            {
                uint32_t frameSyncPos = 0;

                if (bufferPos > buffer + kuDataBlockSize - Mp3PacketHeader::kuFrameHeaderSizeBytes) /* prevent memory overrun */
                {
                    res = SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                    break;
                }

                res = getFrameSyncPosition(bufferPos, kuDataBlockSize - (uint32_t)(bufferPos - buffer), &frameSyncPos);
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    break;
                }
                if (frameSyncPos > 0) /* Gaps between the frames? */
                {
                    res = SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                    break;
                }
                bufferPos += frameSyncPos;

                res = frameHeader.parse(bufferPos);
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    break;
                }

                res = frameHeader.getPacketSizeBytes(&frameSizeBytes);
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    break;
                }

                bufferPos += frameSizeBytes; /* skip to the next expected frame position */
            }

            sce::HeatWave::MemoryHandler::freeCb(buffer);

            return res;
        }

        Mp3Codec::Mp3Codec(DataSource *pSource)
            : AudioCodec(pSource),
            m_decoder(NULL),
            m_ringBuffer(NULL),
            m_nSkipSamples(0)
        {
        }

        Mp3Codec::~Mp3Codec()
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
        * @brief Initialise the decoder, prime for processing data
        */
        SceHwError Mp3Codec::init()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = isMp3Format(m_dataSource);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Setup the platform specific decoder */
            m_decoder = DecoderMp3::createMp3DecoderInstance();
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

            res = parseMp3Header(&m_buffer, &m_header);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (!DecoderMp3::isAvailable(m_header.numChannels))
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;

            res = m_decoder->init(&m_header, m_dataSource);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_decoder->getType() == DecoderMp3::ePull)
            {
                MemoryHandler::freeCb(m_buffer.pData); /* decoder will be handling access of the data */
                m_buffer.pData = NULL;
                m_buffer.uSizeBytes = 0;
            }
            else if (m_decoder->getType() == DecoderMp3::ePushPull)
            {
                m_ringBuffer = new RingBuffer(kuDataBlockSize); /* so we can read in cache friendly blocks from the data source */
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Seek within the audio data to the supplied milli-second offset.
        *
        * @param[in] nSeekOffset The milli-second offset.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Mp3Codec::performSeek(int32_t nSeekOffset)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            int32_t nNumSamplesSkipped = 0;
            int32_t nTocEntry = 0;
            const float kfSecsToMs = 1000.0f;

            if (m_decoder == NULL) {
                res = createDecoder();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            int32_t nSampleOffset = (int32_t)(nSeekOffset * ((float)getSampleRate() / kfSecsToMs)); //convert milli-secs to a byte position.

            if (nSeekOffset == 0)
            {
                if (m_ringBuffer != NULL)
                {
                    m_ringBuffer->reset();
                }
                m_dataSource->seek(0, SEEK_SET);

                res = m_decoder->reset();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                return SCE_HEATWAVE_API_OK;
            }

            // Find the entry which contains the sample we want to seek to

            // Is the sample we want in the last entry?
            if (nSampleOffset > m_seekTable[m_seekTable.size() - 1].nSampleOffset)
            {
                nTocEntry = (int32_t)m_seekTable.size() - 1;
            }

            for (int32_t i = 1; i < (int32_t)m_seekTable.size(); i++)
            {
                if (nSampleOffset < m_seekTable[i].nSampleOffset)
                {
                    nTocEntry = i - 1;
                    break;
                }
            }

            nNumSamplesSkipped = m_seekTable[nTocEntry].nSampleOffset;
            m_nSkipSamples = nSampleOffset - nNumSamplesSkipped; // number of additional samples that will need to be skipped once data is decoded.

            m_dataSource->seek(m_seekTable[nTocEntry].nByteOffset, SEEK_SET);

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
        * @brief Decode uSizeBytes as 16bit PCM data.
        *
        * @param[in] pBuffer Destination for the decoded audio data.
        * @param[in] uSizeBytes The size of pBuffer.
        * @param[out] pNumBytesReadOut The number of bytes written to pBuffer.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Mp3Codec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
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

                if (numBytesDecoded < numBytesRequested && m_decoder->getType() == DecoderMp3::ePushPull) /* Give the decoder more data */
                {
                    res = passBitStreamBlockToDecoder();

                    if (SCE_HEATWAVE_API_FAILED(res))
                    {
                        return res;
                    }
                }

                // decrease skip samples to complete any previous seeks.
                if (m_nSkipSamples > 0)
                {
                    const int32_t knSamplesDecoded = numBytesDecoded / sizeof(int16_t) / m_numberOfChannels;
                    const int32_t knSampleDecrement = m_nSkipSamples < knSamplesDecoded ? m_nSkipSamples : knSamplesDecoded;
                    m_nSkipSamples -= knSampleDecrement;
                    numBytesDecoded -= knSampleDecrement * sizeof(int16_t) * m_numberOfChannels;
                }

                // update offsets with what is left after skip.
                if (m_nSkipSamples == 0)
                {
                    outBuffer += numBytesDecoded / sizeof(int16_t);
                    numBytesRequested -= numBytesDecoded;
                    *pNumBytesReadOut += numBytesDecoded;
                }

                timeoutcounter--;
                if (timeoutcounter == 0) /* prevent any potential for soft locks due to decoder failures */
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass a block of the MP3 data to the decoder
        */
        SceHwError Mp3Codec::passBitStreamBlockToDecoder()
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

        /**
        * @brief Parse the MP3 header and cache information about the audio stream.
        *
        * @param[in] pBuffer    Buffer to read data into.
        * @param[out] pHeader   Destination for the parsed MP3 header of the first MP3 frame.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Mp3Codec::parseMp3Header(BitStream *pBuffer, Mp3PacketHeader *pHeader)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            unsigned char* pcHeader = NULL;
            uint32_t uHeaderPosition = 0;
            uint32_t uId3Size = 0;
            const float kfBytesToKiloBits = 0.001f * 8.0f;
            uint32_t uPacketSizeBytes = 0;
            uint32_t uFramesPerPacket = 0;
            int32_t nAudioPacketOffset = 0;
            int32_t nXingPos = 0;
            int32_t nVbriPos = 0;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pHeader == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* Read in a block of data */
            m_dataSource->seek(0, SEEK_SET);
            m_dataSource->read(m_buffer.pData, 1, pBuffer->uSizeBytes);

            /* Skip ID3 at start of the data */
            if (isId3Tag((unsigned char*)m_buffer.pData))
            {
                uId3Size = getId3Size((unsigned char*)m_buffer.pData);

                m_dataSource->seek(uId3Size, SEEK_SET);
                m_dataSource->read(m_buffer.pData, sizeof(unsigned char), kuDataBlockSize);
            }

            // Find the first MP3 packet
            nRet = getFrameSyncPosition((unsigned char*)m_buffer.pData, kuDataBlockSize, &uHeaderPosition);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }
            pcHeader = (unsigned char*)&m_buffer.pData[uHeaderPosition];

            // Parse the header
            nRet = pHeader->parse(pcHeader);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* cache channel count and sample rate */
            m_numberOfChannels = pHeader->numChannels;
            m_sampleRate = pHeader->samplingFrequency;

            pHeader->getPacketSizeBytes(&uPacketSizeBytes);
            pHeader->getPacketSizeFrames(&uFramesPerPacket);
            m_nSamplesPerPacket = uFramesPerPacket;

            // Calculate duration and create a seek table.
            nXingPos = findXingPos(m_buffer.pData, uPacketSizeBytes);
            nVbriPos = checkForVbri(m_buffer.pData, uPacketSizeBytes);

            // Check if the first MP3 packet is a VBRI or Xing frame (should be the 1st packet if they do exist).
            // If we find either, use the information for the duration and seek table.
            if (nXingPos != NOT_FOUND)
            {
                const char* pXing = m_buffer.pData + nXingPos;
                const int32_t knXingSizeBytes = uPacketSizeBytes - nXingPos;
                const int32_t knFileSizeMp3Packets = readXingDuration(pXing, knXingSizeBytes);

                m_nDuration = _packetsToMillisecs(knFileSizeMp3Packets, m_nSamplesPerPacket, m_sampleRate);

                readXingSeekTable(pXing, knXingSizeBytes);

                nAudioPacketOffset = uPacketSizeBytes;
            }
            else if (nVbriPos != NOT_FOUND)
            {
                const char* pVbri = m_buffer.pData + nVbriPos;
                const int32_t knVbriSizeBytes = uPacketSizeBytes - nVbriPos;

                const int32_t knFileSizeMp3Packets = readVBRIDuration(pVbri, knVbriSizeBytes);
                m_nDuration = _packetsToMillisecs(knFileSizeMp3Packets, m_nSamplesPerPacket, m_sampleRate);

                readVbriSeekTable(pVbri, knVbriSizeBytes);

                nAudioPacketOffset = uPacketSizeBytes;
            }
            else
            {
                // There is no data for the duration of the file:
                // Take the size of the audio stream, and infer a duration (this could be wildly inaccurate for VBR, 
                // but we *should* already know the duration from a tag).
                uint32_t uFileSize = 0;
                int32_t nNumPackets = 0;

                m_dataSource->seek(0, SEEK_END);
                uFileSize = m_dataSource->tell() - uId3Size;

                m_nDuration = (int32_t)(SECS_TO_MS * ((float)uFileSize * kfBytesToKiloBits) / (float)pHeader->bitRate);

                nNumPackets = uFileSize / uPacketSizeBytes;

                for (int32_t i = 0; i < nNumPackets; i++)
                {
                    m_seekTable.push_back(SeekEntry(i * uPacketSizeBytes, i * uFramesPerPacket));
                }
            }

            // Update the seek offsets to skip ID3, etc.
            for (int32_t i = 0; i < (int32_t)m_seekTable.size(); i++)
            {
                m_seekTable[i].nByteOffset += uId3Size + nAudioPacketOffset;
            }

            m_dataSource->seek(0, SEEK_SET);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Return the VBRI duration.
        *
        * @param[in] pBuffer    The full MP3 packet.
        * @param[in] nSizeBytes The byte size of pBuffer.
        *
        * @return The number of MP3 packets in the file, 0 if the MP3 packet is not a VBRI frame.
        */
        int32_t Mp3Codec::readVBRIDuration(const char* pBuffer, const int32_t nSizeBytes)
        {
            const int32_t knFrameCountPos = 14; // The frame count is always a 4byte int starting at byte 14

            if (pBuffer == 0 || nSizeBytes < (int32_t)(knFrameCountPos + sizeof(int32_t)))
            {
                return 0;
            }

            return _bigToPlatformEndian32(*(int32_t*)&pBuffer[knFrameCountPos]);
        }

        /**
        * @brief Return the Xing duration.
        *
        * @param[in] pBuffer    The full MP3 packet.
        * @param[in] nSizeBytes The byte size of pBuffer.
        *
        * @return The number of MP3 packets in the file, 0 if the MP3 frame is not a Xing packet.
        */
        int32_t Mp3Codec::readXingDuration(const char* pBuffer, const int32_t nSizeBytes)
        {
            const int32_t knFlagDurationFrames = 0x0001; // Flag to indicate frame duration data is present.
            const int32_t knDurationOffset = 8; // Duration is always position at byte 8.
            const int32_t knFlagsPos = 4; // Flags are always a 4byte int immediately after the header id.
            const int32_t knFlags = _bigToPlatformEndian32(*(int*)&pBuffer[knFlagsPos]);
            int32_t nMp3Packets = 0;

            if (pBuffer == NULL || nSizeBytes == 0)
            {
                return 0;
            }

            if ((knFlags & knFlagDurationFrames) == knFlagDurationFrames)
            {
                nMp3Packets = _bigToPlatformEndian32(*(int*)&pBuffer[knDurationOffset]);
            }

            return nMp3Packets;
        }

        /**
        * @brief Return byte position of VBRI packet within pBuffer.
        *
        * @param[in] pBuffer The buffer of audio data.
        * @param[in] nSizeBytes The size of pBuffer.
        *
        * @note pBuffer should be the data immediately following the ID3 information.
        *
        * @return The byte position of VBRI packet within pBuffer, NOT_FOUND if there is no VBRI packet.
        */
        int32_t Mp3Codec::checkForVbri(const char* pBuffer, const int32_t nSizeBytes)
        {
            const int32_t knVrbiPos = 32 + 4; // VBRI is *always* 32 bytes after the header (+4 to skip the mp3 sync)

            if (pBuffer == NULL || nSizeBytes == 0)
            {
                return NOT_FOUND;
            }
            if (nSizeBytes < knVrbiPos + 4)
            {
                return NOT_FOUND;
            }

            if (pBuffer[knVrbiPos] == 'V' &&
                m_buffer.pData[knVrbiPos + 1] == 'B' &&
                m_buffer.pData[knVrbiPos + 2] == 'R' &&
                m_buffer.pData[knVrbiPos + 3] == 'I')
            {
                return knVrbiPos;
            }

            return NOT_FOUND;
        }

        /**
        * @brief Read the VBRI seek table.
        *
        * @param[in] pBuffer The VBRI packet.
        * @param[in] nSizeBytes The size of the packet.
        *
        * @return true for success, false otherwise.
        */
        bool Mp3Codec::readVbriSeekTable(const char* pBuffer, const int32_t nSizeBytes)
        {
            if (pBuffer == NULL || nSizeBytes == 0)
            {
                return false;
            }

            // Seek table information is at set offsets.
            const int32_t knEntryCountOffset = 18;
            const int32_t knScaleFactorOffset = 20;
            const int32_t knEntrySizeOffset = 22;
            const int32_t knPacketPerEnrtyOffset = 24;
            const int32_t knTocTableOffset = 26;

            // number of entries in the table.
            const int32_t knNumTocEntries = _bigToPlatformEndian16(*(int16_t*)&pBuffer[knEntryCountOffset]);

            if (knNumTocEntries > 0)
            {
                const int32_t nTocScaleFactor = _bigToPlatformEndian16(*(int16_t*)&pBuffer[knScaleFactorOffset]);
                const int32_t nTocEntrySize = _bigToPlatformEndian16(*(int16_t*)&pBuffer[knEntrySizeOffset]);
                const int32_t nPacketsPerTocEntry = _bigToPlatformEndian16(*(int16_t*)&pBuffer[knPacketPerEnrtyOffset]);
                const int32_t knSamplesPerTocEntry = nPacketsPerTocEntry * m_nSamplesPerPacket;
                
                int32_t nSampleOffset = knSamplesPerTocEntry;
                int32_t nByteOffset = 0;

                for (int32_t i = 0; i < knNumTocEntries; i++, nSampleOffset += knSamplesPerTocEntry)
                {
                    const int32_t knPos = knTocTableOffset + (i * nTocEntrySize); // position of the entry within pBuffer
                    int32_t nEntry = 0;

                    if (nTocEntrySize == sizeof(int16_t))
                    {
                        nEntry = _bigToPlatformEndian16(*(int16_t*)&pBuffer[knPos]);
                    }
                    else if (nTocEntrySize == sizeof(uint32_t))
                    {
                        nEntry = _bigToPlatformEndian32(*(uint32_t*)&pBuffer[knPos]);
                    }

                    nEntry *= nTocScaleFactor;

                    nByteOffset += nEntry;
                    m_seekTable.push_back(SeekEntry(nByteOffset, nSampleOffset));
                }
            }

            return true;
        }

        /**
        * @brief Return the position of Xing packet within pBuffer.
        *
        * @param[in] pBuffer The buffer of audio data.
        * @param[in] nSizeBytes The size of pBuffer.
        *
        * @note pBuffer should be the data immediately following the ID3 information.
        *
        * @return The byte position of Xing packet within pBuffer, NOT_FOUND if there is no Xing packet.
        */
        int32_t Mp3Codec::findXingPos(const char* pBuffer, const int32_t nSizeBytes)
        {
            int32_t nXingPos = 4; // skip the MP3 packet sync header
            const int32_t knMaxXingPos = nXingPos + 32; // Xing should be within 32 bytes

            if (pBuffer == NULL || nSizeBytes == 0)
            {
                return NOT_FOUND;
            }
            if (nSizeBytes < knMaxXingPos)
            {
                return NOT_FOUND;
            }

            // Position of 'Xing' after the header will vary, but it's not clear exactly how.
            // It will be within 32 bytes, so do a scan up to the last point it could be at.
            for (int32_t i = 0; i < knMaxXingPos; i++, nXingPos++)
            {
                // 'Xing' or 'Info' are both valid
                if ((pBuffer[nXingPos] == 'X' && pBuffer[nXingPos + 1] == 'i' &&  // 'Xing'
                    pBuffer[nXingPos + 2] == 'n' && pBuffer[nXingPos + 3] == 'g')
                    || (pBuffer[nXingPos] == 'I' && pBuffer[nXingPos + 1] == 'n' && // 'Info'
                        pBuffer[nXingPos + 2] == 'f' && pBuffer[nXingPos + 3] == 'o'))

                {
                    return nXingPos;
                }
            }

            return NOT_FOUND;
        }

        /**
        * @brief Read the Xing seek table.
        *
        * @param[in] pBuffer    Buffer containing the Xing packet.
        * @param[in] nSizeBytes The byte size of the packet.
        *
        * @return true for success, false otherwise.
        */
        bool Mp3Codec::readXingSeekTable(const char* pBuffer, const int32_t nSizeBytes)
        {
            // Flags
            const int32_t knFlagDurationFrames = 0x0001;    // Frame duration data is present.
            const int32_t knFlagBytesFrame = 0x0002;        // Byte size data is present.
            const int32_t knFlagTOC = 0x0004;               // Seek table is present.

            const int32_t knFlagsPos = 4; // Flags are always a 4byte int immediately after the header id.
            const int32_t knNumEntries = 100; // Xing seek table is always 100 entries.


            int32_t nXingPos = 4; // Skip the MP3 packet header.
            int32_t nByteOffset = 0;
            int32_t nMp3Frames = 0;
            int32_t nNumBytes = 0;
            int32_t nNumSamples = 0;

            if (pBuffer == NULL || nSizeBytes == 0)
            {
                return false;
            }

            const int32_t nFlags = _bigToPlatformEndian32(*(int*)&pBuffer[knFlagsPos]);
            nByteOffset += 4;

            // Check that flags are set for the information needed.
            if ( !((nFlags & knFlagDurationFrames) == knFlagDurationFrames) ||
                !((nFlags & knFlagBytesFrame) == knFlagBytesFrame) ||
                !((nFlags & knFlagTOC) == knFlagTOC)
                )
            {
                return false;
            }

            nMp3Frames = _bigToPlatformEndian32(*(int*)&pBuffer[nXingPos + nByteOffset]);
            nByteOffset += 4;

            nNumSamples = nMp3Frames * m_nSamplesPerPacket;

            nNumBytes = _bigToPlatformEndian32(*(int*)&pBuffer[nXingPos + nByteOffset]);
            nByteOffset += 4;

            for (int32_t i = 0; i < knNumEntries; i++, nByteOffset++)
            {
                const unsigned char entry = pBuffer[nXingPos + nByteOffset];
                float interp = (float)entry / (float)UCHAR_MAX; // interpolation of the full frame and byte sizes

                // nRem is the number of samples to make the the entry align to the number of samples per packet.
                // The next MP3 packet found after the byte position will be at the store sample position.
                const int32_t nRem = m_nSamplesPerPacket - (int32_t)(interp * nNumSamples) % m_nSamplesPerPacket;
                m_seekTable.push_back(SeekEntry((int32_t)(interp * nNumBytes), int32_t((interp * nNumSamples) + nRem)));
            }

            return true;
        }

        SceHwError Mp3Codec::createDecoder()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            /* Setup the platform specific decoder */
            m_decoder = DecoderMp3::createMp3DecoderInstance();
            if (m_decoder == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            res = m_decoder->init(&m_header, m_dataSource);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_decoder->getType() == DecoderMp3::ePull)
            {
                MemoryHandler::freeCb(m_buffer.pData); /* decoder will be handling access of the data */
                m_buffer.pData = NULL;
                m_buffer.uSizeBytes = 0;
            }
            else if (m_decoder->getType() == DecoderMp3::ePushPull)
            {
                m_ringBuffer = new RingBuffer(kuDataBlockSize); /* so we can read in cache friendly blocks from the data source */
            }

            return res;
        }

    } /* HeatWave */
} /* sce */
