/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "MP3\heatwave_api_decoder_mp3_ps3.h"

#include <cell\sysmodule.h>
#include <sys\synchronization.h>
#include "heatwave_api_ringbuffer.h"
#include "heatwave_api_mp3codec.h"

namespace sce{
    namespace HeatWave{

        static const uint32_t kuLibAdecThreadPriority = 100;
        static const uint32_t kuLibAdexThreadStackSize = 16 * 1024;
        static const uint32_t kuLibAdecAlignment = 128;
        static const uint32_t kuDecodeWait = 0;
        static const uint32_t kuDecodeComplete = 1;

        /**
        * @brief Handler for callbacks from libAdec
        */
        static int32_t _adec_callback_msg(CellAdecHandle handle, CellAdecMsgType msgType, uint32_t msgData, void *callbackArg)
        {
            DecoderMp3PS3 *decoder = (DecoderMp3PS3*)callbackArg;

            switch (msgType)
            {
            case CELL_ADEC_MSG_TYPE_AUDONE:
                break;
            case CELL_ADEC_MSG_TYPE_PCMOUT:
                decoder->SetDecodeCompleteFlag(kuDecodeComplete);
                break;
            case CELL_ADEC_MSG_TYPE_ERROR:
                break;
            case CELL_ADEC_MSG_TYPE_SEQDONE:
                break;
            }

            return 0;
        }

        DecoderMp3* DecoderMp3::createMp3DecoderInstance()
        {
            return new DecoderMp3PS3();
        }

        SceHwError DecoderMp3::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int32_t ret = CELL_OK;

            ret = cellSysmoduleLoadModule(CELL_SYSMODULE_ADEC_MP3);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3::unloadCodecLibraries()
        {
            int32_t ret = CELL_OK;

            ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_ADEC);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderMp3::isAvailable(int32_t nChannelNum)
        {
            return true;
        }

        DecoderMp3PS3::DecoderMp3PS3()
            : DecoderMp3(),
            m_decHandle(NULL),
            m_decodeCompleteFlag(0),
            m_pcmBuffer(NULL),
            m_pcmBufferReadPosition(NULL),
            m_pcmBufferEnd(NULL)
        {
            m_resource.ppuThreadPriority = kuLibAdecThreadPriority;
            m_resource.spuThreadPriority = kuLibAdecThreadPriority;
            m_resource.ppuThreadStackSize = kuLibAdexThreadStackSize;
            m_resource.startAddr = NULL;

            m_param.bw_pcm = MP3_BW_16BIT;
        }

        DecoderMp3PS3::~DecoderMp3PS3()
        {
            releaseInternal();
        }

        /**
        * @brief Flag to indicate libAdec has fully decoded the supplied audio data
        *
        * @see ::fillInternal()
        */
        void DecoderMp3PS3::SetDecodeCompleteFlag(uint32_t uFlag)
        {
            sys_mutex_lock(m_decodeMutex, 0);

            m_decodeCompleteFlag = uFlag;
            sys_cond_signal(m_decodeCond);

            sys_mutex_unlock(m_decodeMutex);
        }

        SceHwError DecoderMp3PS3::initInternal()
        {
            int32_t ret = CELL_OK;
            CellAdecType dec_type;
            CellAdecAttr dec_attr;
            CellAdecCb   reg_cb;
            sys_mutex_attribute_t mutex_attr;
            sys_cond_attribute_t cond_attr;

            /* create a mutex to allow blocking whilst libAdec performs the decode */
            sys_mutex_attribute_initialize(mutex_attr);
            sys_cond_attribute_initialize(cond_attr);

            ret = sys_mutex_create(&m_decodeMutex, &mutex_attr);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = sys_cond_create(&m_decodeCond, m_decodeMutex, &cond_attr);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* setup decoder instance */
            dec_type.audioCodecType = CELL_ADEC_TYPE_MP3;

            ret = cellAdecQueryAttr(&dec_type, &dec_attr);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_resource.startAddr = MemoryHandler::mallocCb(dec_attr.workMemSize);
            if (m_resource.startAddr == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }
            m_resource.totalMemSize = dec_attr.workMemSize;

            reg_cb.callbackFunc = (CellAdecCbMsg)_adec_callback_msg;
            reg_cb.callbackArg = this;

            ret = cellAdecOpen(&dec_type, &m_resource, &reg_cb, &m_decHandle);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = cellAdecStartSeq(m_decHandle, &m_param);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* buffer to cache the decoded data */
            m_pcmBuffer = (char*)MemoryHandler::memalignCb(kuLibAdecAlignment, sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame);
            if (m_pcmBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            m_pcmBufferReadPosition = m_pcmBuffer; /* initialise cache as empty */
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3PS3::releaseInternal()
        {
            if (m_decHandle != NULL)
            {
                cellAdecEndSeq(m_decHandle);
                cellAdecClose(m_decHandle);
                m_decHandle = NULL;
            }

            if (m_resource.startAddr != NULL)
            {
                MemoryHandler::freeCb(m_resource.startAddr);
                m_resource.startAddr = NULL;
                m_resource.totalMemSize = 0;
            }

            if (m_pcmBuffer != NULL)
            {
                MemoryHandler::freeCb(m_pcmBuffer);
                m_pcmBuffer = NULL;
                m_pcmBufferEnd = NULL;
                m_pcmBufferReadPosition = NULL;
            }
        }

        /**
        * @brief Decode the provided bit stream, cache the decoded data for use in ::renderInternal()
        */
        SceHwError DecoderMp3PS3::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            int32_t ret = CELL_OK;
            Mp3PacketHeader header;
            const CellAdecPcmItem *pcmitem;
            uint32_t frameByteSize = 0;

            *puBytesConsumed = 0;

            if (m_pcmBufferReadPosition < m_pcmBufferEnd) /* Don't overwrite already decoded data */
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            /* Get a single MP3 frame from pBuffer and pass to the decoder */
            res = header.parse((unsigned char*)pBuffer->pData);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = header.getPacketSizeBytes(&frameByteSize);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_info.startAddr = pBuffer->pData;
            m_info.size = frameByteSize;
            m_info.userData = NULL;
            m_info.pts.lower = CELL_ADEC_PTS_INVALID;
            m_info.pts.upper = CELL_ADEC_PTS_INVALID;

            SetDecodeCompleteFlag(kuDecodeWait);

            ret = sys_mutex_lock(m_decodeMutex, 0); ///< lock ready for sys_cond_wait
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = cellAdecDecodeAu(m_decHandle, &m_info);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* block whilst the decode is performed by libAdec (blocking behaviour matches other platforms) */
            while (m_decodeCompleteFlag != kuDecodeComplete)
            {
                ret = sys_cond_wait(m_decodeCond, 0);
                if (ret != CELL_OK)
                {
                    sys_mutex_unlock(m_decodeMutex);
                    return SCE_HEATWAVE_API_ERROR;
                }
            }
            ret = sys_mutex_unlock(m_decodeMutex);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *puBytesConsumed = m_info.size;

            /* Cache the decoded PCM data for use in ::renderInternal() */
            ret = cellAdecGetPcmItem(m_decHandle, &pcmitem);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            MemoryHandler::memsetCb(m_pcmBuffer, 0, sizeof(int16_t) * m_mp3Info.numChannels * kuMaxSamplesPerMp3Frame);
            ret = cellAdecGetPcm(m_decHandle, m_pcmBuffer);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pcmBufferEnd = m_pcmBuffer + pcmitem->size;
            m_pcmBufferReadPosition = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Provide decoded pcm data by copying from the internal cache of decoded data
        */
        SceHwError DecoderMp3PS3::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
        {
            uint32_t numBytesAvailable = (m_pcmBufferEnd - m_pcmBufferReadPosition); /* Amount of decoded pcm data available in the cache */
            char *out = (char*)pDecodedDataOut;

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

            /** Copy into output but with flipped endian (MultiStream expects little endian input data, 
            * see MultiStreamSynthChannel::initPlayerStream call to cellMSStreamSetInfo)
            */
            for (uint32_t i = 0; i < *pNumBytesOut / sizeof(int16_t); i++)
            {
                *out++ = *(m_pcmBufferReadPosition + 1);
                *out++ = *m_pcmBufferReadPosition;

                m_pcmBufferReadPosition += sizeof(int16_t);
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Reset the context, clear cache of decoded pcm data
        */
        SceHwError DecoderMp3PS3::resetInternal()
        {
            int32_t ret = CELL_OK;

            /* End the previous decode sequence */
            ret = cellAdecEndSeq(m_decHandle);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* start a new decode sequence */
            ret = cellAdecStartSeq(m_decHandle, &m_param);
            if (ret != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* reset the cache of decoded pcm data */
            m_pcmBufferReadPosition = m_pcmBuffer;
            m_pcmBufferEnd = m_pcmBuffer;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
