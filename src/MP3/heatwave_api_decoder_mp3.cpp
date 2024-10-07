/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_MP3

#include "MP3\heatwave_api_decoder_mp3.h"

#include "heatwave_api_ringbuffer.h"
#include "heatwave_api_mp3codec.h"

namespace sce{
    namespace HeatWave{

        DecoderMp3::DecoderMp3(DecoderMp3Type eType)
            : MemoryHandler(),
            m_dataSource(NULL),
            m_decoderType(eType)
        {
        }

        DecoderMp3::~DecoderMp3()
        {
        }

        /**
        * @brief Setup ready for processing
        */
        SceHwError DecoderMp3::init(Mp3PacketHeader *pHeader, DataSource *pSource)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pHeader == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_mp3Info = *pHeader;
            m_dataSource = pSource;

            res = initInternal();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3::release()
        {
            releaseInternal();
        }

        /**
        * @brief Pass a block of data to be decoded.
        *
        * @param[in] pBuffer The block of data to decode.
        * @param[out] puBytesConsumed The number of bytes of data used by the decoder.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError DecoderMp3::fill(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            uint32_t mp3PacketPosition = 0;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (puBytesConsumed == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (getType() != DecoderMp3::ePushPull)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            // Find a MP3 packet by finding an MP3 frame sync flag
            res = Mp3Codec::getFrameSyncPosition((unsigned char*)pBuffer->pData, pBuffer->uSizeBytes, &mp3PacketPosition);
            if (mp3PacketPosition > 0)
            {
                *puBytesConsumed += mp3PacketPosition; // skip to the MP3 packet
                return SCE_HEATWAVE_API_OK;
            }
            else if (SCE_HEATWAVE_API_FAILED(res))
            {
                if (res == SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS)
                {
                    *puBytesConsumed = pBuffer->uSizeBytes;
                    return SCE_HEATWAVE_API_OK;
                }
                return res;
            }
            else
            {   // Check that this is an MP3 packet.
                // Calculate the size of the packet, and verify that the next frame sync matches the predicted position.
                Mp3PacketHeader packetHeader;
                uint32_t nPacketSizeBytes;
                uint32_t nNextPacketPos;

                packetHeader.parse((unsigned char*)pBuffer->pData);
                packetHeader.getPacketSizeBytes(&nPacketSizeBytes);

                res = Mp3Codec::getFrameSyncPosition((unsigned char*)pBuffer->pData + nPacketSizeBytes, pBuffer->uSizeBytes - nPacketSizeBytes, &nNextPacketPos);
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    if (res == SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS)
                    {
                        *puBytesConsumed = pBuffer->uSizeBytes;
                        return SCE_HEATWAVE_API_OK;
                    }
                    else
                    {
                        return res;
                    }
                }

                // The predicted and actual position of the next MP3 packet frame sync do not match - skip to the next frame sync.
                // The current frame sync is not an MP3 packet.
                if (nNextPacketPos != 0)
                {
                    *puBytesConsumed += nNextPacketPos;
                    return SCE_HEATWAVE_API_OK;
                }
            }

            res = fillInternal(pBuffer, puBytesConsumed);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Decode audio data as 16bit PCM
        */
        SceHwError DecoderMp3::decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pDecodedDataOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pNumBytesOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pNumBytesOut = 0;

            res = renderInternal(pDecodedDataOut, uSizeBytes, pNumBytesOut);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Reset the decoder (clear accumulated information the decoder may hold)
        */
        SceHwError DecoderMp3::reset()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = resetInternal();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @return 
            ePushPull - the decoder requires encoded data to be fed in as a bit stream - ::fill()
            ePull - the decoder manages access of encoded data internally
        */
        DecoderMp3::DecoderMp3Type DecoderMp3::getType() const
        {
            return m_decoderType;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */
