/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_AT9

#include "ATRAC9\heatwave_api_decoder_at9.h"

#include "heatwave_api_ringbuffer.h"
#include "heatwave_api_at9codec.h"

namespace sce{
    namespace HeatWave{

        DecoderAt9::DecoderAt9(DecoderAt9Type eType)
            : MemoryHandler(),
            m_dataSource(NULL),
            m_decoderType(eType)
        {
        }

        DecoderAt9::~DecoderAt9()
        {
        }

        /**
        * @brief Setup ready for processing
        */
        SceHwError DecoderAt9::init(DataSource *pSource, uint32_t uNumberOfChannels, uint32_t uSampleRate, int32_t nConfigData)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_numberOfChannels = uNumberOfChannels;
            m_sampleRate = uSampleRate;
            m_configData = nConfigData;
            m_dataSource = pSource;

            res = initInternal();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderAt9::release()
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
        SceHwError DecoderAt9::fill(BitStream *pBuffer, uint32_t *puBytesConsumed)
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

            if (getType() != DecoderAt9::ePushPull)
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
        SceHwError DecoderAt9::decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        SceHwError DecoderAt9::getSeekOffset(uint32_t uTime, uint32_t *puOffset)
        {
            return getSeekOffsetInternal(uTime, puOffset);
        }

        /**
        * @brief Reset the decoder (clear accumulated information the decoder may hold)
        */
        SceHwError DecoderAt9::reset()
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
        DecoderAt9::DecoderAt9Type DecoderAt9::getType() const
        {
            return m_decoderType;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT9 */
