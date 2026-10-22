/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_MP3

#include "MP3\heatwave_api_decoder_mp3_psvita.h"

#include <sceerror.h>
#include "heatwave_api_ringbuffer.h"

#if defined SCE_HEATWAVE_CODEC_MP3_PSVITA_SOFT

#pragma comment(lib, "libSceAudiocodec_stub_weak.a")
#pragma comment(lib, "_libSceSysmodule_stub.a")

namespace sce{
    namespace HeatWave{

        DecoderMp3* DecoderMp3::createMp3DecoderInstance()
        {
            return new DecoderMp3PSVita();
        }

        SceHwError DecoderMp3::loadCodecLibraries(int32_t nMaxChannelNum)
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

        SceHwError DecoderMp3::unloadCodecLibraries()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_AUDIOCODEC);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderMp3::isAvailable(int32_t nChannelNum)
        {
            return true;
        }

        DecoderMp3PSVita::DecoderMp3PSVita()
            : DecoderMp3(),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            MemoryHandler::memsetCb(&m_ctrl, 0, sizeof(SceAudiocodecCtrl));
        }

        DecoderMp3PSVita::~DecoderMp3PSVita()
        {
            releaseInternal();
        }

        SceHwError DecoderMp3PSVita::initInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiocodecQueryMemSize(&m_ctrl, SCE_AUDIOCODEC_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_ctrl.pWorkMem = MemoryHandler::mallocCb(m_ctrl.neededWorkMem);
            if (m_ctrl.pWorkMem == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            sceRes = sceAudiocodecInit(&m_ctrl, SCE_AUDIOCODEC_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* buffer to hold the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::memalignCb(SCE_AUDIODEC_ALIGNMENT_SIZE, SCE_AUDIODEC_ROUND_UP(sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame));
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise to empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3PSVita::releaseInternal()
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
        SceHwError DecoderMp3PSVita::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
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

            sceRes = sceAudiocodecDecode(&m_ctrl, SCE_AUDIOCODEC_MP3);
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
        SceHwError DecoderMp3PSVita::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        SceHwError DecoderMp3PSVita::resetInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiocodecInit(&m_ctrl, SCE_AUDIOCODEC_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */

#else

#pragma comment(lib, "libSceAudiodec_stub.a")

namespace sce{
    namespace HeatWave{

        static int32_t s_nChannelInUseCountPsvMp3 = 0;
        static int32_t s_nChannelMaxCountPsvMp3 = 0;

        DecoderMp3* DecoderMp3::createMp3DecoderInstance()
        {
            return new DecoderMp3PSVita();
        }

        SceHwError DecoderMp3::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int32_t sceRes = SCE_OK;
            SceAudiodecInitParam audiodecInitParam;

            if (nMaxChannelNum > 0) {

                if (nMaxChannelNum > SCE_AUDIODEC_MP3_MAX_STREAMS)
                    nMaxChannelNum = SCE_AUDIODEC_MP3_MAX_STREAMS;

                MemoryHandler::memsetCb(&audiodecInitParam, 0, sizeof(audiodecInitParam));

                audiodecInitParam.size = sizeof(audiodecInitParam.mp3);
                audiodecInitParam.mp3.totalStreams = nMaxChannelNum;
                s_nChannelMaxCountPsvMp3 = nMaxChannelNum;

                sceRes = sceAudiodecInitLibrary(SCE_AUDIODEC_TYPE_MP3, &audiodecInitParam);
                if (sceRes != SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3::unloadCodecLibraries()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecTermLibrary(SCE_AUDIODEC_TYPE_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            s_nChannelMaxCountPsvMp3 = 0;

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderMp3::isAvailable(int32_t nChannelNum)
        {
            if (s_nChannelInUseCountPsvMp3 + nChannelNum <= s_nChannelMaxCountPsvMp3)
                return true;

            return false;
        }

        DecoderMp3PSVita::DecoderMp3PSVita()
            : DecoderMp3(),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            MemoryHandler::memsetCb(&m_ctrl, 0, sizeof(SceAudiodecCtrl));
            MemoryHandler::memsetCb(&m_info, 0, sizeof(SceAudiodecInfo));

            m_info.size = sizeof(SceAudiodecInfoMp3);

            m_ctrl.size = sizeof(SceAudiodecCtrl);
            m_ctrl.wordLength = SCE_AUDIODEC_WORD_LENGTH_16BITS;
            m_ctrl.pInfo = &m_info;
        }

        DecoderMp3PSVita::~DecoderMp3PSVita()
        {
            releaseInternal();
        }

        SceHwError DecoderMp3PSVita::initInternal()
        {
            int32_t sceRes = SCE_OK;

            m_info.mp3.ch = m_mp3Info.numChannels;
            m_info.mp3.version = m_mp3Info.version;

            s_nChannelInUseCountPsvMp3 += m_info.mp3.ch;

            sceRes = sceAudiodecCreateDecoder(&m_ctrl, SCE_AUDIODEC_TYPE_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* buffer to hold the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::memalignCb(SCE_AUDIODEC_ALIGNMENT_SIZE, SCE_AUDIODEC_ROUND_UP(sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame));
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise to empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3PSVita::releaseInternal()
        {
            s_nChannelInUseCountPsvMp3 -= m_info.mp3.ch;

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
        SceHwError DecoderMp3PSVita::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
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
        SceHwError DecoderMp3PSVita::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        SceHwError DecoderMp3PSVita::resetInternal()
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

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3_PSVITA_SOFT */

#endif /* SCE_HEATWAVE_CODEC_MP3 */
