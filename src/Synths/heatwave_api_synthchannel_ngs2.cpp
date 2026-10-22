/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synthchannel_ngs2.h"
#include "Synths\heatwave_api_synth_ngs2.h"
#include <sceerror.h>
#include "Synths\heatwave_api_synthchannel_ngs2libaudio3d.h"

namespace sce{
    namespace HeatWave{

        SynthChannel* SynthChannel::createChannel(int32_t nPortDestination)
        {
            if (nPortDestination == SCE_HEATWAVE_API_OUTPUT_AUDIO3D)
            {
                return new NGS2LibAudio3dSynthChannel();
            }
            return new Ngs2SynthChannel();
        }

        Ngs2SynthChannel::Ngs2SynthChannel()
            : SynthChannel(),
            m_state(kSfxState_Ready),
            m_ngs2VoiceHandle(SCE_NGS2_HANDLE_INVALID),
            m_bytesQueued(0),
            m_synth(NULL),
            m_fLevels(NULL),
            m_nDestination(SCE_HEATWAVE_API_OUTPUT_MAIN)
        {
        }

        Ngs2SynthChannel::~Ngs2SynthChannel()
        {
        }

        /**
        * @brief Get an NGS2 voice and setup ready for playback
        */
        SceHwError Ngs2SynthChannel::initSynthChannel(PlatformSynth *pSynth, InitParams *pParam)
        {
            SceHwError  res = SCE_HEATWAVE_API_OK;

            if (pSynth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam->uNumChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam->uSampleRate == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_synth = (Ngs2Synth*)pSynth;
            if (m_synth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* get a voice */
            res = m_synth->getNgs2SamplerVoice(pParam->nDestinationPort, &m_ngs2VoiceHandle);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_ngs2Format.waveformType   = SCE_NGS2_WAVEFORM_TYPE_PCM_I16L;
            m_ngs2Format.numChannels    = pParam->uNumChannels;
            m_ngs2Format.sampleRate     = pParam->uSampleRate;
            m_ngs2Format.frameOffset    = 0;
            m_ngs2Format.frameMargin    = 0;

            /* setup for the expected format */
            res = setupNgs2Voice(m_ngs2VoiceHandle, &m_ngs2Format, 0);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = m_synth->patchNgs2VoiceToDestination(m_ngs2VoiceHandle, pParam->nDestinationPort);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            m_nDestination = pParam->nDestinationPort;

            /* Setup a matrix containing speaker volumes per source channel */
            m_fLevels = (float32_t*)MemoryHandler::mallocCb(m_ngs2Format.numChannels * m_numMatrixChannels * sizeof(float32_t));
            if (m_fLevels == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Tidy up resources and release the voice back to synth for re-use
        */
        SceHwError Ngs2SynthChannel::releaseSynthChannel()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            Ngs2ProcessBuffer *processing = NULL;

            /* Make sure that the voice is not processing anything (we are about to clear the waveform blocks!) */
            if (m_ngs2VoiceHandle != SCE_NGS2_HANDLE_INVALID)
            {
                int32_t ngs2Result = sceNgs2VoiceKickEvent(m_ngs2VoiceHandle, SCE_NGS2_VOICE_EVENT_KILL);
                if (ngs2Result < SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            /* give back any buffers that the synthchannel was using */
            while (!m_ngs2ProcessingBuffers.empty())
            {
                processing = m_ngs2ProcessingBuffers.front();
                if (processing != NULL)
                {
                    m_processedBuffers.push(processing->pBuffer);
                    m_ngs2ProcessingBuffers.pop();
                    delete processing;
                }
            }

            /* return the ngs2 voice to the synth */
            if (m_synth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_synth->releaseNgs2Voice(m_ngs2VoiceHandle, m_nDestination);

            m_state             = kSfxState_Ready;
            m_ngs2VoiceHandle   = SCE_NGS2_HANDLE_INVALID;
            m_bytesQueued       = 0;

            if (m_fLevels != NULL)
            {
                MemoryHandler::freeCb(m_fLevels);
                m_fLevels = NULL;
            }

            return res;
        }

        /**
        * @brief Pass any SynthBuffers the ngs2 voice has processed into the processed queue.  Add queued SynthBuffers as waveformblocks to the ngs2voice
        */
        SceHwError Ngs2SynthChannel::update()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = SynthChannel::update();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = updateState();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Pass SynthBuffers as waveformblocks to the ngs2 voice */
            res = consumeBuffers();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* SynthBuffers that have been consumed by the ngs2 voice, pass on to the consumed queued */
            res = processBuffers();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Commence processing of data.
        *
        * If the voice is paused a resume is performed, otherwise playback is begun.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2SynthChannel::play()
        {
            int32_t nSceRet = SCE_OK;
            const uint32_t uEventId  = (m_state == kSfxState_Paused ? SCE_NGS2_VOICE_EVENT_RESUME : SCE_NGS2_VOICE_EVENT_PLAY); ///< Resume if the NGS2Voice is paused
            uint32_t uStateFlags = 0;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nSceRet = sceNgs2VoiceGetStateFlags(m_ngs2VoiceHandle, &uStateFlags);
            if (nSceRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (uEventId == SCE_NGS2_VOICE_EVENT_PLAY)
            {
                SceHwError nSceRet = SCE_HEATWAVE_API_OK;

                /* add some buffers to play */
                nSceRet = consumeBuffers();
                if (SCE_HEATWAVE_API_FAILED(nSceRet))
                {
                    return nSceRet;
                }

                /* voice was already playing, just starved of data */
                if ((uStateFlags & SCE_NGS2_VOICE_STATE_FLAG_INUSE) && (uStateFlags & SCE_NGS2_VOICE_STATE_FLAG_EMPTY))
                {
                    m_state = kSfxState_Playing;
                    return SCE_HEATWAVE_API_OK;
                }
            }

            nSceRet = sceNgs2VoiceKickEvent(m_ngs2VoiceHandle, uEventId);
            if (nSceRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_state = kSfxState_Playing;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::stop()
        {
            int32_t ngs2Result = SCE_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2VoiceKickEvent(m_ngs2VoiceHandle, SCE_NGS2_VOICE_EVENT_STOP);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* reset the channel state */
            m_bytesQueued = 0;
            m_state = kSfxState_Ready;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::pause()
        {
            int32_t ngs2Result = SCE_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2VoiceKickEvent(m_ngs2VoiceHandle, SCE_NGS2_VOICE_EVENT_PAUSE);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_state = kSfxState_Paused;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::getState(SceHwSfxState *pOutState)
        {
            if (pOutState == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutState = m_state;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels)
        {
            int32_t ngs2Result  = SCE_OK;

            const uint32_t kNumLevels       = uNumChannels;
            const uint32_t kNumSourceChans  =  m_ngs2Format.numChannels;

            if (pVol == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (uNumChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_synth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (uNumChannels != m_numMatrixChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* Specify the destination channel volume for each channel of the source data */
            for (uint32_t destchan = 0; destchan < uNumChannels; destchan++)
            {
                for(uint32_t sourcechan = 0; sourcechan < kNumSourceChans; sourcechan++)
                {
                    m_fLevels[(destchan * kNumSourceChans) + sourcechan] = pVol[destchan];
                }
            }

            ngs2Result = sceNgs2VoiceSetMatrixLevels(m_ngs2VoiceHandle, 0, m_fLevels, kNumLevels * kNumSourceChans);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2VoiceSetPortMatrix(m_ngs2VoiceHandle, 0, 0); /* flush the updated volume matrix through to the port */
            if (ngs2Result < SCE_OK)    
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::setPitch(float32_t fPitch)
        {
            int32_t ngs2Result = SCE_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2SamplerVoiceSetPitch(m_ngs2VoiceHandle, fPitch);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::setLowpassCutoff(float32_t fLowpassCutoffFactor)
        {
            SceHwError res = SCE_OK;

            const float32_t kfQ         = 1.0f;
            const float32_t kfGain      = 1.0f;
            const float32_t cutoffFreq  = (m_ngs2Format.sampleRate / 2.0f) * fLowpassCutoffFactor;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = setNgs2VoiceFilterByFcq(m_ngs2VoiceHandle, 0, SCE_NGS2_SAMPLER_FILTER_LOCATION_REAR, SCE_NGS2_FILTER_TYPE_LOW_PASS_ONEPOLE, 0, cutoffFreq, kfQ, kfGain);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Sampler voice specific setup
        */
        SceHwError Ngs2SynthChannel::setupNgs2Voice(SceNgs2Handle hVoiceHandle, SceNgs2WaveformFormat *pWaveFormat, uint32_t uFlags)
        {
            int32_t ret = SCE_OK;

            if (hVoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pWaveFormat == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            ret = sceNgs2SamplerVoiceSetup(m_ngs2VoiceHandle, pWaveFormat, 0);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Sampler voice specific filter set
        */
        SceHwError Ngs2SynthChannel::setNgs2VoiceFilterByFcq(SceNgs2Handle hVoiceHandle, uint32_t uIndex, uint32_t uLocation, uint32_t uType, uint32_t uChannelMask, float fC, float fQ, float fLevel)
        {
            int32_t ret = SCE_OK;

            ret = sceNgs2SamplerVoiceSetFilterByFcq(hVoiceHandle, uIndex, uLocation, uType, uChannelMask, fC, fQ, fLevel);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Sampler voice specific waveform block add
        */
        SceHwError Ngs2SynthChannel::addNgs2VoiceWaveformBlocks(SceNgs2Handle hVoiceHandle, const void *pData, const SceNgs2WaveformBlock *pBlock, uint32_t uNumBlocks, uint32_t uFlags)
        {
            int32_t ret = SCE_OK;

            ret = sceNgs2SamplerVoiceAddWaveformBlocks(hVoiceHandle, pData, pBlock, uNumBlocks, uFlags);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Sampler voice specific query decoded data size
        */
        SceHwError Ngs2SynthChannel::getNgs2VoiceDecodedDataSize(SceNgs2Handle hVoiceHandle, uint64_t *pDecodedDataSizeOut)
        {
            int32_t ret = SCE_OK;
            SceNgs2SamplerVoiceState voiceState;

            MemoryHandler::memsetCb(&voiceState, 0, sizeof(SceNgs2SamplerVoiceState));

            ret = sceNgs2SamplerVoiceGetState(hVoiceHandle, &voiceState);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pDecodedDataSizeOut = voiceState.decodedDataSize;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass any SynthBuffers to the ngs2voice to process
        */
        SceHwError Ngs2SynthChannel::consumeBuffers()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            while (m_buffersToProcess.size() > 0)
            {
                SynthBuffer *buffer = m_buffersToProcess.front();
                if (buffer != NULL)
                {
                    SceNgs2WaveformBlock    addBlock;
                    Ngs2ProcessBuffer       *processing = NULL;

                    addBlock.dataOffset     = 0;
                    addBlock.dataSize       = buffer->uDataSizeBytes;
                    addBlock.numSamples     = buffer->uDataSizeBytes / (m_ngs2Format.numChannels * sizeof(int16_t));
                    addBlock.numSkipSamples = 0;
                    addBlock.numRepeats     = 0;
                    addBlock.userData       = 0;

                    /* Add all blocks as CONTINUOUS (no way to know which SynthBuffer is the last) */
                    /* ::getState() will mark the SynthChannel as finished when ngs2 voice runs out of waveformblocks to process */
                    res = addNgs2VoiceWaveformBlocks(m_ngs2VoiceHandle, buffer->pData, &addBlock, 1, SCE_NGS2_WAVEFORM_BLOCK_FLAG_CONTINUOUS);
                    if (SCE_HEATWAVE_API_FAILED(res))
                    {
                        return res;
                    }

                    m_bytesQueued += buffer->uDataSizeBytes; /* Keep track of how many bytes have been added to the ngs2 voice so we can calculate when a block is finished */

                    processing = new Ngs2ProcessBuffer();
                    if (processing == NULL)
                    {
                        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
                    }

                    processing->uByteOffset = m_bytesQueued; /* The synthbuffer will be consumed once the ngs2 voice has processed >= m_bytesQueued of data*/
                    processing->pBuffer = buffer;
                    m_ngs2ProcessingBuffers.push(processing);

                    m_buffersToProcess.pop();
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass any consumed SynthBuffers to the processed queue
        */
        SceHwError Ngs2SynthChannel::processBuffers()
        {
            uint64_t decodedDataSize = 0;
            Ngs2ProcessBuffer *processing   = NULL;
            SceHwError ret = SCE_HEATWAVE_API_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = getNgs2VoiceDecodedDataSize(m_ngs2VoiceHandle, &decodedDataSize);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            /* We can guarentee that m_ngs2ProcessingBuffers is in order - ::consumeBuffers() */
            while (!m_ngs2ProcessingBuffers.empty())
            {
                processing = m_ngs2ProcessingBuffers.front();
                if (processing != NULL)
                {
                    if (decodedDataSize >= processing->uByteOffset) /* We've decoded data beyond this block */
                    {
                        m_processedBuffers.push(processing->pBuffer); /* Setup to pass the SynthBuffer back now the Ngs2SynthChannel is finished with it */
                        m_ngs2ProcessingBuffers.pop();
                        delete processing;
                    }
                    else
                    {
                        return SCE_HEATWAVE_API_OK;
                    }
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2SynthChannel::updateState()
        {
            uint32_t    ngs2StateFlags = 0;
            int32_t     ngs2Result = SCE_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2VoiceGetStateFlags(m_ngs2VoiceHandle, &ngs2StateFlags);
            if (ngs2StateFlags == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (ngs2StateFlags & SCE_NGS2_VOICE_STATE_FLAG_EMPTY) /* ngs2 has processed all the waveform blocks we passed to it (::consumeBuffers()) */
            {
                m_state = kSfxState_Ready;
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
