/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_AT3

#include "ATRAC3\heatwave_api_decoder_at3_psvita.h"

#include <sceerror.h>
#include "heatwave_api_ringbuffer.h"

#pragma comment(lib, "libSceAudiocodec_stub_weak.a")
#pragma comment(lib, "_libSceSysmodule_stub.a")

namespace sce{
    namespace HeatWave{

        DecoderAt3* DecoderAt3::createAt3DecoderInstance()
        {
            return new DecoderAt3PSVita();
        }

        SceHwError DecoderAt3::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int32_t sceRes = SCE_OK;

            if (sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_AUDIOCODEC) != SCE_OK)
            {
                sceRes = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_AUDIOCODEC);
                if (sceRes != SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderAt3::unloadCodecLibraries()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_AUDIOCODEC);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderAt3::isAvailable(int32_t nChannelNum)
        {
            return true;
        }

        DecoderAt3PSVita::DecoderAt3PSVita()
            : DecoderAt3(),
            m_sceCodecType(0),
            m_pcmBlockSize(0),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            MemoryHandler::memsetCb(&m_ctrl, 0, sizeof(SceAudiocodecCtrl));
        }

        DecoderAt3PSVita::~DecoderAt3PSVita()
        {
            releaseInternal();
        }

        SceHwError DecoderAt3PSVita::initInternal()
        {
            int32_t sceRes = SCE_OK;

            switch (m_codecType) {
            case eType_AT3P:
                m_sceCodecType = SCE_AUDIOCODEC_ATX;
                m_ctrl.atx.codecParam1 = m_param1;
                m_ctrl.atx.codecParam2 = m_param2;
                break;
            case eType_AT3:
                m_sceCodecType = SCE_AUDIOCODEC_AT3;
                m_ctrl.at3.codecParam = getAt3ConfigPSP2(m_param1, m_param2);
                break;
            }

            sceRes = sceAudiocodecQueryMemSize(&m_ctrl, m_sceCodecType);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            uint32_t memsize = 0x8000;
            if (m_ctrl.neededWorkMem > memsize)
            {
                memsize = 0x10000;
            }

            m_ctrl.pWorkMem = MemoryHandler::mallocCb(memsize);
            if (m_ctrl.pWorkMem == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            if (m_sceCodecType == SCE_AUDIOCODEC_ATX)
            {
                m_ctrl.unk_atx_0C = 1;
            }

            sceRes = sceAudiocodecInit(&m_ctrl, m_sceCodecType);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (m_sceCodecType == SCE_AUDIOCODEC_AT3)
            {
                switch (m_ctrl.at3.codecParam)
                {
                case 4:
                    m_ctrl.inputEsSize = 384;
                    break;
                case 6:
                    m_ctrl.inputEsSize = 304;
                    break;
                case 0xB:
                    m_ctrl.inputEsSize = 192;
                    break;
                case 0xE:
                case 0xF:
                default:
                    return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
                }
            }

            sceRes = sceAudiocodecQueryPcmBlocksize(&m_ctrl, m_sceCodecType, &m_pcmBlockSize);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* buffer to hold the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::memalignCb(SCE_AUDIODEC_ALIGNMENT_SIZE, SCE_AUDIODEC_ROUND_UP(m_pcmBlockSize));
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise to empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderAt3PSVita::releaseInternal()
        {
            if (m_pcmBuffer)
            {
                MemoryHandler::freeCb(m_pcmBuffer);
                m_pcmBuffer = NULL;
            }

            if (m_ctrl.pWorkMem != NULL)
            {
                MemoryHandler::freeCb(m_ctrl.pWorkMem);
            }

            MemoryHandler::memsetCb(&m_ctrl, 0, sizeof(SceAudiocodecCtrl));
        }

        /**
        * @brief Decode the provided bit stream, cache the decoded data for use in ::renderInternal()
        */
        SceHwError DecoderAt3PSVita::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
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
            m_ctrl.pPcm = m_pcmBuffer;

            sceRes = sceAudiocodecDecode(&m_ctrl, m_sceCodecType);
            if (sceRes != SCE_OK)
            {
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
        SceHwError DecoderAt3PSVita::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        SceHwError DecoderAt3PSVita::resetInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiocodecInit(&m_ctrl, m_sceCodecType);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderAt3PSVita::getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset)
        {
            const float kfSecsToMs = 1000.0f;

            *puOffset = 0;

            uint32_t offsetInPcmSamples = (uint32_t)((float)uTime * ((float)kuSamplingRate / kfSecsToMs));
            uint32_t seekEsNum = (uint32_t)(offsetInPcmSamples / m_pcmBlockSize);

            *puOffset = (seekEsNum * m_ctrl.inputEsSize);

            return SCE_HEATWAVE_API_OK;
        }

        int32_t DecoderAt3PSVita::getAt3ConfigPSP2(uint32_t cmode, uint32_t nbytes)
        {
            if ((cmode == 0) && (nbytes == 0xc0)) {
                return 4;
            }
            if ((cmode == 0) && (nbytes == 0x98)) {
                return 6;
            }
            if ((cmode == 1) && (nbytes == 0x60)) {
                return 0xb;
            }
            if ((cmode == 2) && (nbytes == 0xc0)) {
                return 0xe;
            }
            if ((cmode == 2) && (nbytes == 0x98)) {
                return 0xf;
            }
            return -1;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_AT3 */
