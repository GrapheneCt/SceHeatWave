/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_AT3

#include "ATRAC3\heatwave_api_decoder_at3.h"

#include "heatwave_api_ringbuffer.h"
#include "heatwave_api_at3codec.h"

namespace sce{
    namespace HeatWave{

        DecoderAt3::DecoderAt3(DecoderAt3Type eType)
            : MemoryHandler(),
            m_dataSource(NULL),
            m_decoderType(eType)
        {
        }

        DecoderAt3::~DecoderAt3()
        {
        }

        /**
        * @brief Setup ready for processing
        */
        SceHwError DecoderAt3::init(DataSource *pSource, DecoderAt3CodecType uCodecType, uint8_t uParam1, uint8_t uParam2)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_codecType = uCodecType;
            m_param1 = uParam1;
            m_param2 = uParam2;
            m_dataSource = pSource;

            res = initInternal();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderAt3::release()
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
        SceHwError DecoderAt3::fill(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (puBytesConsumed == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (getType() != DecoderAt3::ePushPull)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
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
        SceHwError DecoderAt3::decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        SceHwError DecoderAt3::getSeekOffset(uint32_t uTime, uint32_t *puOffset)
        {
            return getSeekOffsetInternal(uTime, puOffset);
        }

        /**
        * @brief Reset the decoder (clear accumulated information the decoder may hold)
        */
        SceHwError DecoderAt3::reset()
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
        DecoderAt3::DecoderAt3Type DecoderAt3::getType() const
        {
            return m_decoderType;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT3 */
