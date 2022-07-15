/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "ATRAC9\heatwave_api_decoder_at9_psvita.h"

#include <sceerror.h>
#include <ngs.h>
#include <stdio.h>
#include "heatwave_api_ringbuffer.h"

#pragma comment(lib, "libSceAudiodec_stub.a")

namespace sce{
    namespace HeatWave{

        static int32_t s_nChannelInUseCountPsvAt9 = 0;
        static int32_t s_nChannelMaxCountPsvAt9 = 0;

        DecoderAt9* DecoderAt9::createAt9DecoderInstance()
        {
            return new DecoderAt9PSVita();
        }

        SceHwError DecoderAt9::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int32_t sceRes = SCE_OK;
            SceAudiodecInitParam audiodecInitParam;

            if (nMaxChannelNum > 0) {

                if (nMaxChannelNum > SCE_AUDIODEC_AT9_MAX_CH_IN_LIBRARY)
                    nMaxChannelNum = SCE_AUDIODEC_AT9_MAX_CH_IN_LIBRARY;

                MemoryHandler::memsetCb(&audiodecInitParam, 0, sizeof(audiodecInitParam));

                audiodecInitParam.size = sizeof(audiodecInitParam.at9);
                audiodecInitParam.at9.totalCh = nMaxChannelNum;
                s_nChannelMaxCountPsvAt9 = nMaxChannelNum;

                sceRes = sceAudiodecInitLibrary(SCE_AUDIODEC_TYPE_AT9, &audiodecInitParam);
                if (sceRes != SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderAt9::unloadCodecLibraries()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecTermLibrary(SCE_AUDIODEC_TYPE_AT9);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            s_nChannelMaxCountPsvAt9 = 0;

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderAt9::isAvailable(int32_t nChannelNum)
        {
            if (s_nChannelInUseCountPsvAt9 + nChannelNum <= s_nChannelMaxCountPsvAt9)
                return true;

            return false;
        }

        DecoderAt9PSVita::DecoderAt9PSVita()
            : DecoderAt9(),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            MemoryHandler::memsetCb(&m_ctrl, 0, sizeof(SceAudiodecCtrl));
            MemoryHandler::memsetCb(&m_info, 0, sizeof(SceAudiodecInfo));

            m_info.size = sizeof(SceAudiodecInfoAt9);

            m_ctrl.size = sizeof(SceAudiodecCtrl);
            m_ctrl.wordLength = SCE_AUDIODEC_WORD_LENGTH_16BITS;
            m_ctrl.pInfo = &m_info;
        }

        DecoderAt9PSVita::~DecoderAt9PSVita()
        {
            releaseInternal();
        }

        SceHwError DecoderAt9PSVita::initInternal()
        {
            int32_t sceRes = SCE_OK;

            m_info.at9.ch = m_numberOfChannels;
            m_info.at9.samplingRate = m_sampleRate;
            MemoryHandler::memcpyCb(m_info.at9.configData, &m_configData, sizeof(m_info.at9.configData));

            s_nChannelInUseCountPsvAt9 += m_info.at9.ch;

            sceRes = sceAudiodecCreateDecoder(&m_ctrl, SCE_AUDIODEC_TYPE_AT9);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* buffer to hold the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::memalignCb(SCE_AUDIODEC_ALIGNMENT_SIZE, SCE_AUDIODEC_ROUND_UP(sizeof(int16_t) * m_info.at9.ch * kuMaxSamplesPerAt9Frame));
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise to empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderAt9PSVita::releaseInternal()
        {
            s_nChannelInUseCountPsvAt9 -= m_info.at9.ch;

            sceAudiodecDeleteDecoder(&m_ctrl);

            if (m_pcmBuffer)
            {
                MemoryHandler::freeCb(m_pcmBuffer);
                m_pcmBuffer = NULL;
            }
        }

        /**
        * @brief Decode the provided bit stream, cache the decoded data for use in ::renderInternal()
        */
        SceHwError DecoderAt9PSVita::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            int32_t sceRes = SCE_OK;

            *puBytesConsumed = 0;

            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }
            if (m_pcmBufferReadPosition < m_pcmBufferEnd) /* Don't overwrite already decoded data */
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            m_ctrl.pEs = (SceUInt8*)pBuffer->pData;
            m_ctrl.inputEsSize = pBuffer->uSizeBytes;

            m_ctrl.pPcm = m_pcmBuffer;

            sceRes = sceAudiodecDecode(&m_ctrl);
            if (sceRes != SCE_OK)
            {
                int e = 0;
                sceAudiodecGetInternalError(&m_ctrl, &e);
                printf("decode 0x%X\n", e);
                return SCE_HEATWAVE_API_ERROR;
            }

            /* init the pcm buffer for reading from */
            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer + m_ctrl.outputPcmSize;

            *puBytesConsumed = m_ctrl.inputEsSize;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Provide decoded pcm data by copying from the internal cache of decoded data
        */
        SceHwError DecoderAt9PSVita::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
        {
            uint32_t numBytesAvailable = (m_pcmBufferEnd - m_pcmBufferReadPosition); /* Amount of decoded pcm data available in the cache */

            if (numBytesAvailable < uSizeBytes) /* clip request to the amount of decoded data available */
            {
                *pNumBytesOut = numBytesAvailable;
            }
            else
            {
                *pNumBytesOut = uSizeBytes;
            }

            if (*pNumBytesOut == 0)
            {
                return SCE_HEATWAVE_API_OK;
            }

            MemoryHandler::memcpyCb(pDecodedDataOut, m_pcmBufferReadPosition, *pNumBytesOut); /* copy pcm data into output buffer */
            m_pcmBufferReadPosition += *pNumBytesOut; /* increment read position within the decoded data */

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderAt9PSVita::resetInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecClearContext(&m_ctrl);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderAt9PSVita::getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset)
        {
            const float kfSecsToMs = 1000.0f;
            int32_t res = 0;
            int32_t configData = 0;
            SceNgsAT9SkipBufferInfo info;

            *puOffset = 0;

            uint32_t offsetInPcmSamples = (uint32_t)((float)uTime * ((float)m_info.at9.samplingRate / kfSecsToMs));
            MemoryHandler::memcpyCb(&configData, m_info.at9.configData, sizeof(m_info.at9.configData));

            printf("in: %u\n", uTime);
            res = sceNgsAT9GetSectionDetails(offsetInPcmSamples, 1, configData, &info);
            printf("res: 0x%X\n", info.nStartByteOffset);
            if (res != SCE_NGS_OK)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *puOffset = info.nStartByteOffset;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
