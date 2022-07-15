/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_voiceinternal.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_audiostream.h"
#include "heatwave_api_systeminternal.h"

namespace sce{
    namespace HeatWave{

        uint32_t VoiceInternal::s_globalVoiceCount = 0;

        VoiceInternal::VoiceInternal(SfxInternal *pParent)
            : m_channel(NULL),
            m_loopsRemaining(0),
            m_sourceHandle(NULL),
            m_parentSfx(pParent),
            m_audioStream(NULL)
        {
            for (uint32_t i = 0; i < kuNumBuffers; i++)
            {
                m_buffer[i].pData = NULL;
                m_buffer[i].uDataSizeBytes = 0;
            }

            m_availBuffers.clear();

            VoiceInternal::s_globalVoiceCount++;
        }

        VoiceInternal::~VoiceInternal()
        {
            release();

            VoiceInternal::s_globalVoiceCount--;
        }

        /**
        * @brief Setup the DataStream::Instance and SynthChannel
        */
        bool VoiceInternal::initialise(SourceInternal *pSource, SceHwSfxPlayParam *pParam)
        {
            SystemInternal  *systemHandle = NULL;
            PlatformSynth   *synthHandle  = NULL;
            SceHwError      res = SCE_HEATWAVE_API_OK;
            void            *buffer = NULL;
            SynthChannel::InitParams channelParams;

            if (pSource == NULL)
            {
                return false;
            }
            if (pSource->getAudioStream() == NULL)
            {
                return false;
            }
            if (pParam == NULL)
            {
                return false;
            }

            /* get handles, cache handles that are going to be reused */
            systemHandle = SystemInternal::getInstance();
            if (!systemHandle)
            {
                return false;
            }

            synthHandle = systemHandle->getSynth();
            if (synthHandle == NULL)
            {
                return false;
            }

            m_sourceHandle = pSource;

            /* the data stream that this voice is going to use */
            res = pSource->getAudioStream()->clone(&m_audioStream);
            if (SCE_HEATWAVE_API_FAILED(res) || m_audioStream == NULL)
            {
                return false;
            }

            /* get the synth channel handle */
            channelParams.nDestinationPort = pParam->nDestination;
            channelParams.nA3dPriority = pParam->nAudio3dPriority;
            channelParams.uNumChannels = m_audioStream->getNumChannels();
            channelParams.uSampleRate = m_audioStream->getSampleRate();
            channelParams.bIsAdpcm = m_audioStream->getIsAdpcm();

            res = synthHandle->createChannel(&channelParams, &m_channel);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return false;
            }

            /* create the buffers for reading in the audio data */
            for (uint32_t i = 0; i < kuNumBuffers; i++)
            {
                buffer = MemoryHandler::mallocCb(kuBufferSizeBytes);
                if (buffer == NULL)
                {
                    for (uint32_t j = 0; j < kuNumBuffers; j++) /* Clean up any buffers that did succeed */
                    {
                        if (m_buffer[j].pData != NULL)
                        {
                            MemoryHandler::freeCb(m_buffer[j].pData);
                        }
                        m_buffer[j].uDataSizeBytes = 0;
                    }
                    return false;
                }

                m_buffer[i].pData           = buffer;
                m_buffer[i].uDataSizeBytes  = kuBufferSizeBytes;

                m_availBuffers.push_back(&m_buffer[i]);
            }

            reset();

            return true;
        }

        /**
        * @brief release all resources related to this instance
        */
        void VoiceInternal::release()
        {
            SystemInternal  *systemHandle = NULL;
            PlatformSynth   *synthHandle  = NULL;
            
            systemHandle = SystemInternal::getInstance();
            if (!systemHandle)
            {
                return;
            }

            synthHandle = systemHandle->getSynth();
            if (synthHandle == NULL)
            {
                return;
            }

            /* Release the audio data stream */
            if (m_audioStream != NULL)
            {
                delete m_audioStream;
                m_audioStream = NULL;
            }

            /* Clean up the synth channel */
            if (m_channel != NULL)
            {
                synthHandle->destroyChannel(m_channel);
                m_channel = NULL;
            }

            /* dispose of the buffers */
            for (uint32_t i = 0; i < kuNumBuffers; i++)
            {
                if (m_buffer[i].pData != NULL)
                {
                    MemoryHandler::freeCb(m_buffer[i].pData);
                    m_buffer[i].pData = NULL;
                    m_buffer[i].uDataSizeBytes = 0;
                }
            }

            m_availBuffers.clear();
        }

        /**
        * @brief Prepare for playback, then begin processing of audio data.
        *
        * Queue up buffers to play and trigger the SynthChannel to start outputting data.
        *
        * @return true on success, false otherwise.
        */
        bool VoiceInternal::play()
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            std::vector<SynthChannel::SynthBuffer*>::iterator it = m_availBuffers.begin();

            /* Make sure we have something to play */
            if (m_sourceHandle == NULL || m_audioStream == NULL || m_channel == NULL)
            {
                return false;
            }

            while(it != m_availBuffers.end()) /* Queue the initial set of data */
            {
                if (queueBuffer(*it))
                {
                    it = m_availBuffers.erase(it);
                }
                else
                {
                    it++;
                }
            }

            nRet = applySourceSettings(); /* Flush the SourceInternal settings */
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return false;
            }

            /* Trigger */
            nRet = m_channel->play();
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return false;
            }

            return true;
        }

        bool VoiceInternal::stop()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return false;
            }

            res = m_channel->stop();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return false;
            }

            res = reset();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return false;
            }

            return SCE_HEATWAVE_API_SUCCEEDED(res);
        }

        bool VoiceInternal::pause()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return false;
            }

            res = m_channel->pause();
            return SCE_HEATWAVE_API_SUCCEEDED(res);
        }

        bool VoiceInternal::resume()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return false;
            }

            res = m_channel->play();
            return SCE_HEATWAVE_API_SUCCEEDED(res);
        }

        /**
        * @brief Feed data to SynthChannel as needed.
        *
        * Clean up buffers processed by the SynthChannel, and pass in new buffers as needed.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError VoiceInternal::update()
        {
            SceHwError nRet   = SCE_HEATWAVE_API_OK;
            SynthChannel::SynthBuffer *pFinished = NULL;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = m_channel->update();
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* Tidy up any buffers that the channel has processed and queue a new buffer to replace it */
            nRet = m_channel->popBuffer(&pFinished);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            while (pFinished != NULL)
            {
                if (pFinished->uDataSizeBytes != kuBufferSizeBytes) /* Ran out of data to process, finished */
                {
                    pFinished->uDataSizeBytes = kuBufferSizeBytes;
                    m_availBuffers.push_back(pFinished);
                    return SCE_HEATWAVE_API_OK;
                }

                queueBuffer(pFinished);

                nRet = m_channel->popBuffer(&pFinished);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError VoiceInternal::updateParameters()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->updateParameters();
            return res;
        }

        /**
        * @brief Set the overall gain multiplier
        */
        SceHwError VoiceInternal::setVolume(float32_t fVol)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setGainMultiplier(fVol);
            return res;
        }

        /**
        * @brief Set the overall pitch multiplier
        */
        SceHwError VoiceInternal::setPitch(float32_t fPitchMul)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setPitchMultiplier(fPitchMul);
            return res;
        }

        SceHwError VoiceInternal::setPosition(SceHwVector vPos)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setPosition(vPos);
            return res;
        }

        SceHwError VoiceInternal::setVelocity(SceHwVector vVel)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setVelocity(vVel);
            return res;
        }

        SceHwError VoiceInternal::setForward(SceHwVector vDir)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setForward(vDir);
            return res;
        }

        SceHwError VoiceInternal::setMinMaxDistance(float32_t fMin, float32_t fMax)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setRolloffDistances(fMin, fMax);
            return res;
        }

        SceHwError VoiceInternal::setCone(float32_t fInsideAng, float32_t fOutsideAng, float32_t fOutsideGain, float32_t fOutsideFreq)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->setEmitterCone(fInsideAng, fOutsideAng, fOutsideGain, fOutsideFreq);
            return res;
        }

        /**
        * @brief Pan on the x-axis
        * Achieved by setting the 3d position as relative to the listener and positioning on the x-axis according to fPan
        */
        SceHwError VoiceInternal::setPan(float32_t fPan)
        {
            SceHwError  res = SCE_HEATWAVE_API_OK;
            SceHwVector position;

            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle->type() == eSource3d)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            position.fX = fPan;
            position.fY = 0.0f;
            position.fZ = 0.0f;

            res = m_channel->setAsRelative(true);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = m_channel->setPosition(position);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError VoiceInternal::getType(SceHwSourceType *pTypeOut)
        {
            if (pTypeOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pTypeOut = m_sourceHandle->type();
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError VoiceInternal::getState(SceHwSfxState *pStateOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pStateOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_channel->getState(pStateOut);
            return res;
        }

        /**
        * @brief Seek to the specified playback time.
        *
        * @param[in] nSeekTimeMs The new playback position in milli-seconds.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError VoiceInternal::seek(int32_t nSeekTimeMs)
        {
            const int32_t knLoopCount = m_sourceHandle->loopCount();
            int32_t nLoopsSkipped = 0;
            int32_t nDuration = 0;
            int32_t nSingleLoopDuration = 0;
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            if (nSeekTimeMs < 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_audioStream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }
            nSingleLoopDuration = m_audioStream->getDuration();
            nDuration = nSingleLoopDuration;

            if (m_sourceHandle->loopCount() != knLoopInfinite)
            {
                nDuration *= m_sourceHandle->loopCount() + 1;
            }

            // seeking beyond the end, clip and seek to the end.
            if (nSeekTimeMs > nDuration && m_sourceHandle->loopCount() != knLoopInfinite)
            {
                nSeekTimeMs = nDuration;
                m_loopsRemaining = 0;

                nRet = m_audioStream->seek(nSeekTimeMs);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }
                return SCE_HEATWAVE_API_OK;
            }

            // If seek will skip over loops, decrement the number of remaining loops.
            if (knLoopCount > 0 && knLoopCount != knLoopInfinite)
            {
                nLoopsSkipped = (nSeekTimeMs - (nSeekTimeMs % nSingleLoopDuration)) / nSingleLoopDuration;
                m_loopsRemaining = knLoopCount - nLoopsSkipped;
            }

            nRet = m_audioStream->seek(nSeekTimeMs % nSingleLoopDuration);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Reset to state ready for playback.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError VoiceInternal::reset()
        {
            SynthChannel::SynthBuffer* finished = NULL;

            if (m_audioStream != NULL)
            {
                m_audioStream->reset();
            }

            m_loopsRemaining = m_sourceHandle->loopCount();

            m_channel->popBuffer(&finished);
            while (finished)
            {
                finished->uDataSizeBytes = kuBufferSizeBytes;

                m_channel->popBuffer(&finished);
            }

            m_availBuffers.clear();

            for (uint32_t i = 0; i < kuNumBuffers; i++)
            {
                m_availBuffers.push_back(&m_buffer[i]);
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Flush through the Position, Velocity and Forward to the channel
        */
        SceHwError VoiceInternal::apply3DSettings()
        {
            SceHwError  res = SCE_HEATWAVE_API_OK;
            SceHwVector vec;

            if (m_sourceHandle != NULL && m_sourceHandle->type() == eSource2d)
            {
                return SCE_HEATWAVE_API_OK;
            }
            if (m_channel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_parentSfx->getPosition(&vec);
            res = setPosition(vec);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_parentSfx->getVeloctiy(&vec);
            res = setVelocity(vec);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_parentSfx->getForward(&vec);
            res = setForward(vec);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Flush parameters from SourceInternal to the SynthChannel
        */
        SceHwError VoiceInternal::applySourceSettings()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_parentSfx == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            if (m_sourceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = setVolume(m_parentSfx->getVolume() * m_sourceHandle->volume());
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = setPitch(m_parentSfx->getPitch() * m_sourceHandle->pitch());
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* 3d specific settings */
            if (m_sourceHandle->type() == eSource3d)
            {
                res = setMinMaxDistance(m_sourceHandle->minDistance(), m_sourceHandle->maxDistance());
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                res = setCone(m_sourceHandle->insideAngle(), m_sourceHandle->outsideAngle(), m_sourceHandle->outsideGain(), m_sourceHandle->outsideFreq());
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                res = apply3DSettings();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            } /* 2d sepecific settings */
            else if (m_sourceHandle->type() == eSource2d)
            {
                res = setPan(m_sourceHandle->pan());
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            res = updateParameters();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read in the next buffer of data from DataStream, queue it for the SynthChannel to process.
        *
        * Read a block of data from AudioStream. If less data than requested is received (AudioStream has completed)
        * perform looping as necessary.
        *
        * @param[in] pBuffer    Buffer to fill.
        *
        * @return true if the requested buffer was filled, false when unable to provide the amount of data requested.
        */
        bool VoiceInternal::queueBuffer(SynthChannel::SynthBuffer *pBuffer)
        {
            SceHwError  nRet = SCE_HEATWAVE_API_OK;
            uint32_t    uSizeBytes = 0;
            int16_t     *pnData = NULL;
            uint32_t    uBytesRead = 0;

            if (m_channel == NULL)
            {
                return false;
            }
            if (pBuffer == NULL)
            {
                return false;
            }
            if (pBuffer->pData == NULL || pBuffer->uDataSizeBytes == 0)
            {
                return false;
            }

            pnData        = (int16_t*)pBuffer->pData;
            uSizeBytes   = kuBufferSizeBytes;
            MemoryHandler::memsetCb(pnData, 0, uSizeBytes);

            /* read in 1 stream buffer worth of data from the data */
            uBytesRead = m_audioStream->read(pnData, uSizeBytes);

            if (uBytesRead < uSizeBytes) /* Did we read to the end of the stream (read less than we requested)? */
            {
                if (m_loopsRemaining != 0) /* Should the stream loop? */
                {
                    const uint32_t bytestoread = uSizeBytes - uBytesRead; /* How many bytes to finish filling the buffer? */
                        
                    if (m_loopsRemaining != knLoopInfinite)
                    {
                        m_loopsRemaining--;
                    }

                    nRet = m_audioStream->reset();
                    if (SCE_HEATWAVE_API_FAILED(nRet))
                    {
                        return false;
                    }

                    uBytesRead += m_audioStream->read(pnData + (uBytesRead / sizeof(int16_t)), bytestoread);
                }
            }

            if (uBytesRead == 0) /* No more data to process, finished */
            {
                return false;
            }

            pBuffer->uDataSizeBytes = uBytesRead;

            nRet = m_channel->pushBuffer(pBuffer);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return false;
            }

            return true;
        }

    } /* HeatWave */
} /* sce */
