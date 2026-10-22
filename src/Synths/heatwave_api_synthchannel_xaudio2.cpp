/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synthchannel_xaudio2.h"
#include "heatwave_api_errors.h"

namespace sce{
    namespace HeatWave{

        SynthChannel* SynthChannel::createChannel(int32_t nPortDestination)
        {
            (void)nPortDestination;
            return new XAudio2SynthChannel();
        }

        XAudio2SynthChannel::XAudio2SynthChannel()
            : SynthChannel(),
            m_xaudio2Voice(NULL),
            m_state(kSfxState_Ready),
            m_numChannels(0),
            m_xaudio2MasterVoice(NULL),
            m_synth(NULL),
            m_fLevels(NULL)
        {
        }

        XAudio2SynthChannel::~XAudio2SynthChannel()
        {
        }

        SceHwError XAudio2SynthChannel::initSynthChannel(PlatformSynth *pSynth, InitParams *pParam)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            WAVEFORMATEX xaudio2Format  = {0};
            XAudio2Synth *xaudio2Synth  = NULL;

            if (!pSynth)
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

            xaudio2Synth = (XAudio2Synth*)pSynth;
            if (xaudio2Synth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            /* Convert channel format to WAVEFORMATEX */
            res = getWaveFormatX(pParam->uNumChannels, pParam->uSampleRate, &xaudio2Format);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = xaudio2Synth->getXAudio2Voice(&xaudio2Format, &m_xaudio2Voice);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_numChannels   = pParam->uNumChannels;
            m_synth         = xaudio2Synth;

            /* Setup a matrix containing speaker volumes per source channel */
            m_fLevels = (float32_t*)MemoryHandler::mallocCb(sizeof(float32_t) * m_numChannels * m_numMatrixChannels);
            if (m_fLevels == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::releaseSynthChannel()
        {
            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            m_xaudio2Voice->DestroyVoice();
            m_xaudio2Voice = NULL;

            if (m_fLevels != NULL)
            {
                MemoryHandler::freeCb(m_fLevels);
                m_fLevels = NULL;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::update()
        {
            SceHwError res  = SCE_HEATWAVE_API_OK;
            XAUDIO2_VOICE_STATE xaudio2VoiceState = {0};

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = SynthChannel::update();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = consumeBuffers();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = processBuffers();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_xaudio2Voice->GetState(&xaudio2VoiceState);
            if (xaudio2VoiceState.BuffersQueued == 0) /* No more buffers to process, must be finished */
            {
                stop();
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::play()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            HRESULT xaudio2Result = S_OK;

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = consumeBuffers(); /* prepare some buffers to play */
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            xaudio2Result = m_xaudio2Voice->Start(); /* Start processing IXAudio2SourceVoice queued buffers */
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            m_state = kSfxState_Playing;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::stop()
        {
            HRESULT xaudio2Result = S_OK;

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            xaudio2Result = m_xaudio2Voice->Stop();
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_xaudio2Voice->FlushSourceBuffers(); /* Clear out any queued buffers (Default behaviour of IXAudio2SourceVoice::Start() is to continue processing of buffers from last IXAudio2SourceVoice::Stop()) */
            m_state = kSfxState_Ready;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::pause()
        {
            HRESULT xaudio2Result = S_OK;

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            
            xaudio2Result = m_xaudio2Voice->Stop(); /* Stop processing buffers (queued buffers are left as they are & will resume on IXAudio2SourceVoice::Start()) */
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_state = kSfxState_Paused;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::getState(SceHwSfxState *pOutState)
        {
            if (pOutState == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutState = m_state;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            IXAudio2MasteringVoice  *xaudio2MasterVoice = NULL;
            HRESULT                 xaudio2Result       = S_OK;
            XAUDIO2_VOICE_DETAILS   xaudio2MasterVoiceDetails = {0};

            const uint32_t kNumLevels       = uNumChannels;
            const uint32_t kNumSourceChans  = m_numChannels;

            if (pVol == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_synth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* Make sure that the matrix we receive can be used by the master voice */
            res = m_synth->getXAudio2MasterVoice(&xaudio2MasterVoice);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            xaudio2MasterVoice->GetVoiceDetails(&xaudio2MasterVoiceDetails);

            if (kNumLevels != xaudio2MasterVoiceDetails.InputChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* Apply the same pan matrix to each channel of the source data */
            for(uint32_t i = 0; i < xaudio2MasterVoiceDetails.InputChannels; i++)
            {
                for(uint32_t chan = 0; chan < kNumSourceChans; chan++)
                {
                    m_fLevels[(kNumSourceChans * i) + chan] = pVol[i];
                }
            }

            /* Pass the matrix to the XAudio2 voice */
            xaudio2Result = m_xaudio2Voice->SetOutputMatrix(NULL, kNumSourceChans, kNumLevels, m_fLevels);
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::setPitch(float32_t fPitch)
        {
            HRESULT xaudio2Result = S_OK;

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            xaudio2Result = m_xaudio2Voice->SetFrequencyRatio(fPitch);
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2SynthChannel::setLowpassCutoff(float32_t fLowpassCutoffFactor)
        {
            XAUDIO2_VOICE_DETAILS voiceDetails  = {0};
            XAUDIO2_FILTER_PARAMETERS params;
            UINT32      voiceSampleRate = 0;
            HRESULT     xaudio2Result   = S_OK;
            float32_t   cutoffFreq      = 0.0f;

            const float kfOneOverQ = 1.0f; ///< Use Q of 1.0f

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_xaudio2Voice->GetVoiceDetails(&voiceDetails);
            voiceSampleRate = voiceDetails.InputSampleRate;

            cutoffFreq = fLowpassCutoffFactor * (float32_t)voiceSampleRate / 2.0f;

            params.Type         = LowPassFilter;
            params.Frequency    = fLowpassCutoffFactor;
            params.OneOverQ     = kfOneOverQ;

            xaudio2Result = m_xaudio2Voice->SetFilterParameters(&params);
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Fill out WAVEFORMATEX structure for use by XAudio2
        */
        SceHwError XAudio2SynthChannel::getWaveFormatX(uint32_t uNumChannels, uint32_t uSampleRate, WAVEFORMATEX* pXaudio2FormatOut)
        {
            const uint32_t knWavMinChannels = 1;
            const uint32_t knWavMaxChannels = 2;

            const uint32_t knWavMinSampleRate =  8000;
            const uint32_t knWavMaxSampleRate = 48000;

            if (uNumChannels < knWavMinChannels || uNumChannels > knWavMaxChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (uSampleRate < knWavMinSampleRate || uSampleRate > knWavMaxSampleRate)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pXaudio2FormatOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            pXaudio2FormatOut->nChannels = uNumChannels;
            pXaudio2FormatOut->nSamplesPerSec = uSampleRate;

            pXaudio2FormatOut->wFormatTag = WAVE_FORMAT_PCM;
            pXaudio2FormatOut->nAvgBytesPerSec = (DWORD)(sizeof(int16_t) * uSampleRate * uNumChannels);
            pXaudio2FormatOut->nBlockAlign = (WORD)(sizeof(int16_t) * uNumChannels);
            pXaudio2FormatOut->wBitsPerSample = 16;
            pXaudio2FormatOut->cbSize = 0;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass SynthBuffers of audio data to the XAudio2 source voice for use
        */
        SceHwError XAudio2SynthChannel::consumeBuffers()
        {
            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            while(m_buffersToProcess.size() > 0)
            {
                SynthBuffer *buffer = m_buffersToProcess.front();
                if (buffer != NULL)
                {
                    HRESULT xaudio2Result = S_OK;
                    XAUDIO2_BUFFER xaudio2Buffer    = {0};

                    xaudio2Buffer.AudioBytes = buffer->uDataSizeBytes;
                    xaudio2Buffer.pAudioData = (BYTE*)buffer->pData;

                    xaudio2Result = m_xaudio2Voice->SubmitSourceBuffer(&xaudio2Buffer);
                    if (FAILED(xaudio2Result))
                    {
                        return SCE_HEATWAVE_API_ERROR;
                    }

                    m_xaudio2ProcessingBuffers.push(buffer);
                    m_buffersToProcess.pop();
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pass any buffers that have been consumed by the IXAudio2SourceVoice back to the m_processedBuffers queue
        *
        * @note Order of SynthBuffers in m_xaudio2ProcessingBuffers and consumption by the IXAudio2SourceVoice is guaranteed to be in the order they were added
        * (to the IXAudio2SourceVoice and the m_xaudio2ProcessingBuffers queue)
        */
        SceHwError XAudio2SynthChannel::processBuffers()
        {
            SynthBuffer         *processing         = NULL;
            XAUDIO2_VOICE_STATE xaudio2VoiceState   = {0};

            if (m_xaudio2Voice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_xaudio2Voice->GetState(&xaudio2VoiceState);

            while (!m_xaudio2ProcessingBuffers.empty())
            {
                processing = m_xaudio2ProcessingBuffers.front();
                if (processing != NULL)
                {
                    if (xaudio2VoiceState.BuffersQueued < m_xaudio2ProcessingBuffers.size())
                    {
                        m_processedBuffers.push(processing);
                        m_xaudio2ProcessingBuffers.pop();
                    }
                    else
                    {
                        return SCE_HEATWAVE_API_OK;
                    }
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
