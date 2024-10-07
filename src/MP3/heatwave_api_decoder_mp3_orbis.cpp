/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_MP3

#include "MP3\heatwave_api_decoder_mp3_orbis.h"

#include "heatwave_api_ringbuffer.h"
#include <sceerror.h>
#include <libsysmodule.h>

#pragma comment(lib, "libSceAudiodec_stub_weak.a")

namespace sce{
    namespace HeatWave{

        const int32_t knInvalidlibAudioDecHandle = -1;

        DecoderMp3* DecoderMp3::createMp3DecoderInstance()
        {
            return new DecoderMp3Orbis();
        }

        /**
        * @brief Setup libAudioDec and libAudioDec MP3 decoder
        */
        SceHwError DecoderMp3::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceSysmoduleLoadModule(SCE_SYSMODULE_AUDIODEC);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            sceRes = sceAudiodecInitLibrary(SCE_AUDIODEC_TYPE_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Clean up libAudioDec
        */
        SceHwError DecoderMp3::unloadCodecLibraries()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecTermLibrary(SCE_AUDIODEC_TYPE_MP3);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (sceSysmoduleIsLoaded(SCE_SYSMODULE_AUDIODEC))
            {
                sceRes = sceSysmoduleUnloadModule(SCE_SYSMODULE_AUDIODEC);
                if (sceRes != SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderMp3::isAvailable(int32_t nChannelNum)
        {
            return true;
        }

        DecoderMp3Orbis::DecoderMp3Orbis()
            : DecoderMp3(),
            m_handle(knInvalidlibAudioDecHandle),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            m_param.iBwPcm = SCE_AUDIODEC_WORD_SZ_16BIT;
           
            m_param.uiSize = sizeof(m_param);
            m_info.uiSize = sizeof(m_info);
            m_bst.uiSize = sizeof(m_bst);
            m_pcm.uiSize = sizeof(m_pcm);

            m_ctrl.pParam = &m_param;
            m_ctrl.pBsiInfo = &m_info;
            m_ctrl.pAuInfo = &m_bst;
            m_ctrl.pPcmItem = &m_pcm;
        }

        DecoderMp3Orbis::~DecoderMp3Orbis()
        {
            releaseInternal();
        }

        /**
        * @brief Get a decoder handle from sceLibAudioDec, setup a buffer to decode pcm data into
        */
        SceHwError DecoderMp3Orbis::initInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecCreateDecoder(&m_ctrl, SCE_AUDIODEC_TYPE_MP3);
            if (sceRes < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_handle = sceRes;

            /* buffer to hold the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::mallocCb(sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame);
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcm.pPcmAddr = m_pcmBuffer;
            m_pcm.uiPcmSize = sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame;

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise to empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3Orbis::releaseInternal()
        {
            if (m_handle != knInvalidlibAudioDecHandle)
            {
                sceAudiodecDeleteDecoder(m_handle);
                m_handle = knInvalidlibAudioDecHandle;
            }

            if (m_pcmBuffer)
            {
                MemoryHandler::freeCb(m_pcmBuffer);
                m_pcmBuffer = NULL;
            }
        }

        /**
        * @brief Decode the provided bit stream, cache the decoded data for use in ::renderInternal()
        */
        SceHwError DecoderMp3Orbis::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            int32_t sceRes = SCE_OK;

            if (m_pcmBufferReadPosition < m_pcmBufferEnd) /* Don't overwrite already decoded data */
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            m_bst.pAuAddr = pBuffer->pData;
            m_bst.uiAuSize = pBuffer->uSizeBytes;

            m_pcm.uiPcmSize = sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame; /* max decoded data we expect */

            sceRes = sceAudiodecDecode(m_handle, &m_ctrl);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* init the pcm buffer for reading from */
            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer + m_pcm.uiPcmSize;

            *puBytesConsumed = m_bst.uiAuSize;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Provide decoded pcm data by copying from the internal cache of decoded data
        */
        SceHwError DecoderMp3Orbis::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
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

        /**
        * @brief Reset the context, clear available decoded pcm data
        */
        SceHwError DecoderMp3Orbis::resetInternal()
        {
            int32_t sceRes = SCE_OK;

            sceRes = sceAudiodecClearContext(m_handle);
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

#endif /* SCE_HEATWAVE_CODEC_MP3 */
