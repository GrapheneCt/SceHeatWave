/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_multistream.h"
#include "Synths\heatwave_api_synthchannel_multistream.h"
#include "stdio.h"

namespace sce{
    namespace HeatWave{

        void *_multistreamMemAlign(uint32_t nSize, uint32_t nAlign);
        void _multistreamFree(void *pPtr);

        SynthChannel* SynthChannel::createChannel(int32_t nPortDestination)
        {
            (void)nPortDestination;
            return new MultiStreamSynthChannel();
        }

        MultiStreamSynthChannel::MultiStreamSynthChannel()
            : SynthChannel(),
            m_synth(NULL),
            m_nSampleRate(0),
            m_nMultiStreamVoiceID(-1),
            m_bPaused(false),
            nNumChannels(1),
            pFilterMemBlock(NULL)
        {
        }

        MultiStreamSynthChannel::~MultiStreamSynthChannel()
        {
            if (pFilterMemBlock)
            {
                _multistreamFree(pFilterMemBlock);
                pFilterMemBlock = NULL;
            }
        }

        SceHwError MultiStreamSynthChannel::FillBuffer(int nIndex, bool bConsume)
        {
            if ((nIndex < 0) | (nIndex >= kNumBuffers))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (bConsume)
            {
                SynthBuffer *buffer = m_ngsProcessingBuffers.front();
                m_processedBuffers.push(buffer);
                m_ngsProcessingBuffers.pop();
            }

            if (nIndex == 0)
            {
                if (m_buffersToProcess.empty() == false)
                {
                    SynthBuffer *buffer = m_buffersToProcess.front();
                    cellMSStreamSetFirstRead(m_nMultiStreamVoiceID, buffer->pData, buffer->uDataSizeBytes);
                    m_buffersToProcess.pop();
                    m_ngsProcessingBuffers.push(buffer);
                }
                else
                {
                    cellMSStreamSetFirstRead(m_nMultiStreamVoiceID, NULL, 0);
                }
            }
            else
            {
                if (m_buffersToProcess.empty() == false)
                {
                    SynthBuffer *buffer = m_buffersToProcess.front();
                    cellMSStreamSetSecondRead(m_nMultiStreamVoiceID, buffer->pData, buffer->uDataSizeBytes);
                    m_buffersToProcess.pop();
                    m_ngsProcessingBuffers.push(buffer);
                }
                else
                {
                    cellMSStreamSetSecondRead(m_nMultiStreamVoiceID, NULL, 0);
                }
            }

            return SCE_HEATWAVE_API_OK;
        }
        
        void StreamCallback(int nStreamNumber, void *pUserData, int cType, void *pWriteBuffer, int nBufferSize)
        {
            MultiStreamSynthChannel *pSynthChannel = (MultiStreamSynthChannel *)pUserData;
            CellMSInfo MSInfo;
            int nBufferID;

            cellMSStreamGetInfo(nStreamNumber, &MSInfo);
            
            if (MSInfo.FirstBuffer == pWriteBuffer)
            {
                nBufferID = 0;
            }
            else if (MSInfo.SecondBuffer == pWriteBuffer)
            {
                nBufferID = 1;
            }
            else
            {
                //Incorrect buffer setup?
                return;
            }

            if (cType == CELL_MS_CALLBACK_MOREDATA)
            {
                pSynthChannel->FillBuffer(nBufferID, true); 
            }
            else if ((cType == CELL_MS_CALLBACK_CLOSESTREAM) | (cType == CELL_MS_CALLBACK_FINISHSTREAM))
            {
                pSynthChannel->EmptyBuffer();
            }
        }

        SceHwError MultiStreamSynthChannel::initPlayerStream(int nSampleRate, uint32_t uNumChannels)
        {
            CellMSInfo  MS_Info;
            float fZeroVolume[64] = {0};
            int nError;
            int nMemoryNeeded;
            void *pParamAddr;
            CellMSFXFilter FilterInfo;

            float fActiveVolumes[8] = {
                1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            };

            if ((nSampleRate < 0) | (nSampleRate >= kHighestSampleRate))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if ((uNumChannels < 1) | (uNumChannels > m_synth->kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            MS_Info.SubBusGroup         = CELL_MS_MASTER_BUS; // Which sub bus we are routing through

            m_nSampleRate = nSampleRate;
            this->nNumChannels = uNumChannels;

            MS_Info.Pitch               = m_nSampleRate;
            MS_Info.numChannels         = uNumChannels;
            MS_Info.flags               = CELL_MS_STREAM_NOFLAGS;
            MS_Info.initialOffset       = 0;
            MS_Info.inputType           = CELL_MS_16BIT_LITTLE;

            nError = cellMSStreamSetInfo(m_nMultiStreamVoiceID, &MS_Info);
            if (nError != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            cellMSStreamSetCallbackFunc(m_nMultiStreamVoiceID, StreamCallback);
            cellMSStreamSetCallbackData(m_nMultiStreamVoiceID, this);

            nError = cellMSCoreSetVolume8(m_nMultiStreamVoiceID, CELL_MS_WET, CELL_MS_CHANNEL_0, fActiveVolumes);
            if (nError != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nError = cellMSCoreSetVolume64(m_nMultiStreamVoiceID, CELL_MS_DRY, fZeroVolume);
            if (nError != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nMemoryNeeded = cellMSFXFilterGetNeededMemorySize(1);
            pFilterMemBlock = _multistreamMemAlign(nMemoryNeeded, 128);

            if (pFilterMemBlock == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            cellMSCoreSetDSP( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0, m_synth->nDSPHandle);
            
            pParamAddr = (char *)cellMSCoreGetDSPParamAddr( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0 );

            if (pParamAddr == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            FilterInfo.fFrequency = m_synth->kSampleRate / 2;
            FilterInfo.FilterMode = CELL_MSFX_FILTERMODE_LOWPASS_RESONANT;
            FilterInfo.fResonance= 1.0f;

            nError = cellMSFXFilterInit( pParamAddr, pFilterMemBlock, &FilterInfo);
            if (nError != CELL_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            cellMSCoreBypassDSP( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0, CELL_MS_BYPASSED );

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::initSynthChannel(PlatformSynth *pSynth, InitParams *pParam)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            MultiStreamSynth *pMultiStreamSynth = (MultiStreamSynth*)pSynth;
            
            if (!pMultiStreamSynth)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if ((pParam->uNumChannels == 0) || (pParam->uNumChannels > MultiStreamSynth::kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (pParam->uSampleRate == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_synth = pMultiStreamSynth;

            /* get a voice */
            res = pMultiStreamSynth->getMultiStreamVoice(&m_nMultiStreamVoiceID);
            if(SCE_HEATWAVE_API_FAILED(res))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = initPlayerStream(pParam->uSampleRate, pParam->uNumChannels);
            if(SCE_HEATWAVE_API_FAILED(res))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::releaseSynthChannel()
        {
            SceHwError nReturn;

            if (m_nMultiStreamVoiceID > -1)
            {
                cellMSStreamSetCallbackFunc(m_nMultiStreamVoiceID, NULL);
                cellMSStreamSetCallbackData(m_nMultiStreamVoiceID, NULL);
            }

            nReturn = EmptyBuffer();
            if (nReturn != SCE_HEATWAVE_API_OK)
            {
                return nReturn;
            }

            if (m_nMultiStreamVoiceID > -1)
            {
                nReturn = m_synth->releaseMultiStreamVoice(m_nMultiStreamVoiceID);
                if (nReturn != SCE_HEATWAVE_API_OK)
                {
                    return nReturn;
                }
            }

            if (pFilterMemBlock)
            {
                _multistreamFree(pFilterMemBlock);
                pFilterMemBlock = NULL;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::EmptyBuffer()
        {
            SynthBuffer *pProcessing = NULL;

            while (!m_ngsProcessingBuffers.empty())
            {
                pProcessing = m_ngsProcessingBuffers.front();
                if (pProcessing != NULL)
                {
                    m_processedBuffers.push(pProcessing);
                    m_ngsProcessingBuffers.pop();
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::play()
        {
            SceHwError nReturn;

            if (m_bPaused == true)
            {
                m_bPaused = false;
                cellMSCoreSetPause(m_nMultiStreamVoiceID, CELL_MS_NOTPAUSED);
            }
            else
            {
                nReturn = FillBuffer(0, false);
                if (nReturn != SCE_HEATWAVE_API_OK)
                {
                    return nReturn;
                }

                nReturn = FillBuffer(1, false);
                if (nReturn != SCE_HEATWAVE_API_OK)
                {
                    return nReturn;
                }

                cellMSStreamPlay(m_nMultiStreamVoiceID);
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::stop()
        {
            if (m_nMultiStreamVoiceID > -1)
            {
                cellMSCoreStop(m_nMultiStreamVoiceID, CELL_MS_KEYOFF);
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::pause()
        {
            cellMSCoreSetPause(m_nMultiStreamVoiceID, CELL_MS_PAUSED);
            m_bPaused = true;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::getState(SceHwSfxState *pOutState)
        {
            int nStatus;

            if (pOutState == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_bPaused)
            {
                *pOutState = kSfxState_Paused;
                return SCE_HEATWAVE_API_OK;
            }

            nStatus = cellMSStreamGetStatus(m_nMultiStreamVoiceID);

            if (nStatus & CELL_MS_STREAM_ON)
            {
                *pOutState = kSfxState_Playing;
                return SCE_HEATWAVE_API_OK;
            }

            *pOutState = kSfxState_Ready;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels)
        {
            SceHwError nReturn;

            if (pVol == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (uNumChannels != MultiStreamSynth::kMaxChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nReturn = cellMSCoreSetVolume8(m_nMultiStreamVoiceID, CELL_MS_WET, CELL_MS_CHANNEL_0, pVol);

            return nReturn;
        }

        SceHwError MultiStreamSynthChannel::setPitch(float32_t fPitch)
        {
            int nNewSampleRate = (int)((float)m_nSampleRate * fPitch);

            cellMSStreamSetPitch(m_nMultiStreamVoiceID, nNewSampleRate);

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynthChannel::setLowpassCutoff(float32_t fLowpassCutoffFactor)
        {
            void *pParamAddr;
            CellMSFXFilter FilterInfo;
            int nReturn;

            cellMSCoreSetDSP( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0, m_synth->nDSPHandle);
            pParamAddr = (char *)cellMSCoreGetDSPParamAddr( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0 );

            FilterInfo.fFrequency = kLowestCutoffFrequency + (((m_synth->kSampleRate / 2) - kLowestCutoffFrequency) * fLowpassCutoffFactor);
            FilterInfo.FilterMode = CELL_MSFX_FILTERMODE_LOWPASS_RESONANT;
            FilterInfo.fResonance = 1.0f;

            nReturn = cellMSFXFilterInit( pParamAddr, pFilterMemBlock, &FilterInfo);
            if (nReturn != CELL_OK)
            {
                return SCE_HEATWAVE_API_OK;
            }

            if (fLowpassCutoffFactor != 1.0f)
            {
                cellMSCoreBypassDSP( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0, CELL_MS_NOTBYPASSED );
            }
            else
            {
                cellMSCoreBypassDSP( m_nMultiStreamVoiceID, CELL_MS_DSP_SLOT_0, CELL_MS_BYPASSED );
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
