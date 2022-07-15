/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_ngs.h"
#include "Synths\heatwave_api_synthchannel_ngs.h"
#include <string.h>
#include <stdio.h>

namespace sce{
    namespace HeatWave{

        SynthChannel* SynthChannel::createChannel(int32_t nPortDestination)
        {
            (void)nPortDestination;
            return new NgsSynthChannel();
        }

        NgsSynthChannel::NgsSynthChannel()
            : SynthChannel(),
            m_synth(NULL),
            m_ngsVoiceHandle(NULL),
            VoiceHandle(SCE_NGS_INVALID_HANDLE),
            nVoiceIndex(-1),
            bIsPaused(false)
        {
        }

        NgsSynthChannel::~NgsSynthChannel()
        {
        }

        int NgsSynthChannel::initPlayerStream( SceNgsHVoice hVoice, int nSampleRate, int nNumChannels, bool isAdpcm )
        {
            int                returnCode;
            SceNgsBufferInfo   bufferInfo;
            SceNgsPlayerParams *pPcmParams;

            // Get player parameters
            returnCode = sceNgsVoiceLockParams( hVoice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            // Set player parameters
            memset( bufferInfo.data, 0, bufferInfo.size);
            pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
            pPcmParams->desc.id     = SCE_NGS_PLAYER_PARAMS_STRUCT_ID;
            pPcmParams->desc.size   = sizeof(SceNgsPlayerParams);

            pPcmParams->fPlaybackFrequency          = (SceFloat32)nSampleRate;
            pPcmParams->fPlaybackScalar             = 1.0f;
            pPcmParams->nLeadInSamples              = 0;
            pPcmParams->nLimitNumberOfSamplesPlayed = 0;
            pPcmParams->nChannels                   = nNumChannels;

            if ( pPcmParams->nChannels == 1 ) 
            {
                pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
                pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_LEFT_CHANNEL;
            } 
            else 
            {
                pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
                pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_RIGHT_CHANNEL;
            }

            if (isAdpcm)
                pPcmParams->nType        = SCE_NGS_PLAYER_TYPE_ADPCM;
            else
                pPcmParams->nType        = SCE_NGS_PLAYER_TYPE_PCM;
            pPcmParams->nStartBuffer     = 0;
            pPcmParams->nStartByte       = 0;

            pPcmParams->buffs[0].pBuffer    = NULL;
            pPcmParams->buffs[0].nNumBytes  = 0;
            pPcmParams->buffs[0].nLoopCount = 0;
            pPcmParams->buffs[0].nNextBuff  = 1;
            pPcmParams->buffs[1].pBuffer    = NULL;
            pPcmParams->buffs[1].nNumBytes  = 0;
            pPcmParams->buffs[1].nLoopCount = 0;
            pPcmParams->buffs[1].nNextBuff  = 0;

            // Update player parameters
            returnCode = sceNgsVoiceUnlockParams( hVoice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* Enable the send filters */
            returnCode = sceNgsVoiceBypassModule( hVoice, SCE_NGS_SIMPLE_VOICE_SEND_1_FILTER, SCE_NGS_MODULE_FLAG_NOT_BYPASSED );
            if ( returnCode != SCE_NGS_OK )
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }
        
        SceHwError NgsSynthChannel::initSynthChannel(PlatformSynth *pSynth, InitParams *pParam)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            NgsSynth *pNgsSynth = (NgsSynth*)pSynth;

            if (!pNgsSynth)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            
            if ((pParam->uNumChannels == 0) || (pParam->uNumChannels > NgsSynth::kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (pParam->uSampleRate == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_synth = pNgsSynth;

            res = pNgsSynth->getNgsVoice(pParam->uNumChannels, &VoiceHandle, &nVoiceIndex);
            if(SCE_HEATWAVE_API_FAILED(res))
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            
            res = initPlayerStream(VoiceHandle, pParam->uSampleRate, pParam->uNumChannels, pParam->bIsAdpcm);
            if(SCE_HEATWAVE_API_FAILED(res))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::releaseSynthChannel()
        {
            stop();

            m_synth->releaseNgsVoice(nVoiceIndex);
            EmptyBuffer();

            VoiceHandle = SCE_NGS_INVALID_HANDLE;
            nVoiceIndex = -1;

            return SCE_HEATWAVE_API_OK;
        }

        void streamCallback( const SceNgsCallbackInfo *pCallbackInfo )
        {
            NgsSynthChannel *pSynthChannel = (NgsSynthChannel *)pCallbackInfo->pUserData;
            int nCallbackType = pCallbackInfo->nCallbackData;
            int nBufferID = pCallbackInfo->nCallbackData2;

            if (nCallbackType == SCE_NGS_PLAYER_SWAPPED_BUFFER)
            {
                pSynthChannel->FillBuffer(nBufferID, true);
            }
            else if (nCallbackType == SCE_NGS_PLAYER_END_OF_DATA)
            {
                pSynthChannel->EmptyBuffer();
                //empty the consumed packets
            }

        }
                
        SceHwError NgsSynthChannel::EmptyBuffer()
        {
            SynthBuffer *processing = NULL;
            while (!m_ngsProcessingBuffers.empty())
            {
                processing = m_ngsProcessingBuffers.front();
                if (processing != NULL)
                {
                    m_processedBuffers.push(processing);
                    //printf("empty [0x%x] onto consumed\n", processing);
                    m_ngsProcessingBuffers.pop();
                }
            }
        
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::FillBuffer(int nIndex, bool bConsume)
        {
            int                 returnCode;
            SceNgsBufferInfo    bufferInfo;
            SceNgsPlayerParams  *pPcmParams;
            SynthBuffer         *buffer;

            //bConsume is set when the callback is called, which means a buffer has been consumed and needs to be refilled.
            //bConsume is set to off when it is the initial buffer fill. This means 2 new buffers are being added, but nothing has been consumed (as it isn't playing).
            if (bConsume)
            {
                SynthBuffer *buffer = m_ngsProcessingBuffers.front();
                m_processedBuffers.push(buffer);
                m_ngsProcessingBuffers.pop();
            }

            // Get player parameters
            returnCode = sceNgsVoiceLockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

            if (m_buffersToProcess.empty() == false)
            {
                buffer = m_buffersToProcess.front();

                pPcmParams->buffs[nIndex].nNumBytes = buffer->uDataSizeBytes;
                pPcmParams->buffs[nIndex].pBuffer = buffer->pData;
                pPcmParams->buffs[nIndex].nNextBuff = 1 - nIndex;

                m_buffersToProcess.pop();
                m_ngsProcessingBuffers.push(buffer);
            }
            else
            {
                pPcmParams->buffs[nIndex].nNumBytes = 0;
                pPcmParams->buffs[nIndex].pBuffer = 0;
                pPcmParams->buffs[nIndex].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;
            }

            returnCode = sceNgsVoiceUnlockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::play()
        {
            int returnCode;

            if (bIsPaused == false)
            {

                FillBuffer(0, false);
                FillBuffer(1, false);

                returnCode = sceNgsVoiceSetModuleCallback( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, streamCallback, this );
                if ( returnCode != SCE_NGS_OK ) 
                {
                    return returnCode;
                }

                returnCode = sceNgsVoicePlay( VoiceHandle );
            }
            else
            {
                bIsPaused = false;
                returnCode = sceNgsVoiceResume( VoiceHandle );
            }

            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::stop()
        {
            int returnCode;

            returnCode = sceNgsVoiceSetModuleCallback( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_NO_CALLBACK, NULL );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            returnCode = sceNgsVoiceKeyOff( VoiceHandle );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            bIsPaused = false;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::pause()
        {
            int returnCode;

            returnCode = sceNgsVoicePause( VoiceHandle );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            bIsPaused = true;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::getState(SceHwSfxState *pOutState)
        {
            SceNgsVoiceInfo VoiceInfo;

            if (pOutState == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            sceNgsVoiceGetInfo(VoiceHandle, &VoiceInfo);

            if (bIsPaused)
            {
                *pOutState = kSfxState_Paused;
            }
            else if (VoiceInfo.uVoiceState & SCE_NGS_VOICE_STATE_ACTIVE)
            {
                *pOutState = kSfxState_Playing;
            }
            else if (VoiceInfo.uVoiceState & SCE_NGS_VOICE_STATE_FINALIZING)
            {
                *pOutState = kSfxState_Playing;
            }
            else if (VoiceInfo.uVoiceState & SCE_NGS_VOICE_STATE_AVAILABLE)
            {
                *pOutState = kSfxState_Ready;
            }
            else if (VoiceInfo.uVoiceState & SCE_NGS_VOICE_STATE_UNLOADING)
            {
                //Although the voice is soon not going to be available, there is no "unloading" state in heatwave
                *pOutState = kSfxState_Ready;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels)
        {
            int returnCode;
            SceNgsPatchRouteInfo patchRouteInfo;
            SceNgsHPatch Patch;

            if (pVol == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (uNumChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            Patch = m_synth->GetPatch(nVoiceIndex);

            returnCode = sceNgsPatchGetInfo( Patch, &patchRouteInfo, NULL );

            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if ( patchRouteInfo.nOutputChannels == 1 ) 
            {
                if (uNumChannels == 1)
                {
                    patchRouteInfo.vols.m[0][0] = pVol[0]; // left to left
                    patchRouteInfo.vols.m[0][1] = pVol[0]; // left to right
                }
                else
                {
                    patchRouteInfo.vols.m[0][0] = pVol[0]; // left to left
                    patchRouteInfo.vols.m[0][1] = pVol[1]; // right to right
                }
            } 
            else 
            {
                if (uNumChannels == 1)
                {
                    patchRouteInfo.vols.m[0][0] = pVol[0]; // left to left
                    patchRouteInfo.vols.m[0][1] = 0.0f; // nothing
                    patchRouteInfo.vols.m[1][0] = 0.0f; // nothing
                    patchRouteInfo.vols.m[1][1] = pVol[0]; // left to right
                }
                else
                {
                    patchRouteInfo.vols.m[0][0] = pVol[0]; // left to left
                    patchRouteInfo.vols.m[0][1] = 0.0f; // nothing
                    patchRouteInfo.vols.m[1][0] = 0.0f; // nothing
                    patchRouteInfo.vols.m[1][1] = pVol[1]; // right to right
                }
            }

            returnCode = sceNgsVoicePatchSetVolumesMatrix( Patch, &patchRouteInfo.vols );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            return SCE_HEATWAVE_API_OK;
        }


        SceHwError NgsSynthChannel::setPitch(float32_t fPitch)
        {
            SceNgsBufferInfo   bufferInfo;
            SceNgsPlayerParams *pPcmParams;
            int returnCode;

            // Get player parameters
            returnCode = sceNgsVoiceLockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_OK;
            }

            pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
            pPcmParams->fPlaybackScalar = fPitch;

            returnCode = sceNgsVoiceUnlockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_OK;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynthChannel::setLowpassCutoff(float32_t fLowpassCutoffFactor)
        {
            SceNgsBufferInfo   bufferInfo;
            SceNgsFilterParams *pFilterParams;
            int returnCode;
            uint32_t numChannels = 0;
            const float32_t kfFilterResonance = 1.0f;
            const float32_t kfFilterMinFreq = 20.0f;
            const float32_t kfFilterMaxFreq = 22000.0f;
            const float32_t kfFilterFreqRange = kfFilterMaxFreq - kfFilterMinFreq;
            const SceInt32  kfFilterMode = SCE_NGS_FILTER_LOWPASS_ONEPOLE;

            if ((fLowpassCutoffFactor < 0.0f) || (fLowpassCutoffFactor > 1.0f))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            
            /* Set the send 1 filter */
            returnCode = sceNgsVoiceLockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_SEND_1_FILTER, SCE_NGS_FILTER_PARAMS_STRUCT_ID, &bufferInfo );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            numChannels = bufferInfo.size / sizeof(SceNgsFilterParams);
            pFilterParams = (SceNgsFilterParams *)bufferInfo.data;

            for(uint32_t chan = 0; chan < numChannels; chan++)
            {
                pFilterParams->eFilterMode  = kfFilterMode;
                pFilterParams->fResonance   = kfFilterResonance;
                pFilterParams->fFrequency   = kfFilterMinFreq + (kfFilterFreqRange * fLowpassCutoffFactor);

                pFilterParams++;
            }

            returnCode = sceNgsVoiceUnlockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_SEND_1_FILTER );
            if ( returnCode != SCE_NGS_OK )
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* Set the send 2 filter */
            returnCode = sceNgsVoiceLockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_SEND_2_FILTER, SCE_NGS_FILTER_PARAMS_STRUCT_ID, &bufferInfo );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            numChannels = bufferInfo.size / sizeof(SceNgsFilterParams);
            pFilterParams = (SceNgsFilterParams *)bufferInfo.data;

            for(uint32_t chan = 0; chan < numChannels; chan++)
            {
                pFilterParams->eFilterMode  = kfFilterMode;
                pFilterParams->fResonance   = kfFilterResonance;
                pFilterParams->fFrequency   = kfFilterMinFreq + (kfFilterFreqRange * fLowpassCutoffFactor);

                pFilterParams++;
            }

            returnCode = sceNgsVoiceUnlockParams( VoiceHandle, SCE_NGS_SIMPLE_VOICE_SEND_2_FILTER );
            if ( returnCode != SCE_NGS_OK )
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
