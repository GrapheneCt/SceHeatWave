/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_synthchannelinterface.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_synthinterface.h"
#include "heatwave_api_systeminternal.h"
#include "heatwave_api_panner.h"
#include "heatwave_api.h"

namespace sce{
    namespace HeatWave{

        SynthChannel::SynthChannel()
            : m_volumeMatrix(NULL),
            m_numMatrixChannels(0),
            m_gainMultiplier(1.0f),
            m_pitchMultiplier(1.0f),
            m_panner(NULL),
            m_areParamsDirty(false)
        {
        }

        SceHwError SynthChannel::init(PlatformSynth *pSynth, InitParams *pParam)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            SystemInternal *systemHandle = NULL;

            if (pSynth == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam->uNumChannels == 0 || pParam->uNumChannels > kuMaxChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam->uSampleRate == 0 || pParam->uSampleRate > kuMaxSampleRate)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* Cache the panner handle and setup volume matrices */
            systemHandle = SystemInternal::getInstance();
            if (systemHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_panner = systemHandle->getPanner();
            if (m_panner == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_surroundParams.nPriority = pParam->nA3dPriority;

            flagSurroundParamsAsDirty();

            res = pSynth->getNumOutputChannels(&m_numMatrixChannels); /* Cache the number of output channels the platform specific synth supports */
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (m_numMatrixChannels > 0)
            {
                m_volumeMatrix = (float32_t*)MemoryHandler::mallocCb(sizeof(float32_t) * m_numMatrixChannels);
                if (m_volumeMatrix == NULL)
                {
                    return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
                }

                for (uint32_t i = 0; i < m_numMatrixChannels; i++)
                {
                    m_volumeMatrix[i] = 1.0f;
                }
            }

            /* platform specific setup */
            res = initSynthChannel(pSynth, pParam);
            return res;
        }

        SceHwError SynthChannel::release()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            stop();

            if (m_volumeMatrix != NULL)
            {
                MemoryHandler::freeCb(m_volumeMatrix);
                m_volumeMatrix = NULL;
                m_numMatrixChannels = 0;
            }

            /* platform specific clean up */
            res = releaseSynthChannel();
            return res;
        }


        SceHwError SynthChannel::update()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_areParamsDirty) ///< Nothing to do
            {
                res = updateParameters();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                m_areParamsDirty = false;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Add a buffer to be played (FIFO)
        */
        SceHwError SynthChannel::pushBuffer(SynthBuffer *pBuffer)
        {
            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pBuffer->pData == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pBuffer->uDataSizeBytes == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_buffersToProcess.push(pBuffer);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get a finished buffer (FIFO), NULL if no buffers have been processed since the last call
        */
        SceHwError SynthChannel::popBuffer(SynthBuffer **pOutBuffer)
        {
            if (pOutBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_processedBuffers.size() == 0)
            {
                *pOutBuffer = NULL;
                return SCE_HEATWAVE_API_OK;
            }

            *pOutBuffer = m_processedBuffers.front();
            m_processedBuffers.pop();
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setPosition(SceHwVector vPosition)
        {
            if (m_panner == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_surroundParams.vPosition = vPosition;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setVelocity(SceHwVector vVelocity)
        {
            if (m_panner == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_surroundParams.vVelocity = vVelocity;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setForward(SceHwVector vForward)
        {
            if (m_panner == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_surroundParams.vForward = vForward;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setRolloffDistances(float32_t fMinDistance, float32_t fMaxDistance)
        {
            if (fMinDistance < 0.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fMaxDistance < fMinDistance)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_surroundParams.fMinDistance = fMinDistance;
            m_surroundParams.fMaxDistance = fMaxDistance;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setDistanceFactor(float32_t fFactor)
        {
            if (fFactor < 0.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_surroundParams.fDistanceFactor = fFactor;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setEmitterCone(float32_t fInsideAng, float32_t fOutsideAng, float32_t fOutsideGain, float32_t fOutsideFreq)
        {
            if (fInsideAng < 0.0f || fInsideAng > 360.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fOutsideAng < 0.0f || fOutsideAng > 360.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fInsideAng > fOutsideAng)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fOutsideGain < 0.0f || fOutsideGain > 1.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fOutsideFreq < 0.0f || fOutsideFreq > 1.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_surroundParams.fInsideAngle   = fInsideAng;
            m_surroundParams.fOutsideAngle  = fOutsideAng;
            m_surroundParams.fOutsideGain   = fOutsideGain;
            m_surroundParams.fOutsideFreq   = fOutsideFreq;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setGainMultiplier(float32_t fGain)
        {
            m_gainMultiplier = fGain;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setPitchMultiplier(float32_t fPitch)
        {
            m_pitchMultiplier = fPitch;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Flag SynthChannel as positioned relative to the listener
        * bRelativeToListener == false is relative to the world origin (0,0,0)
        */
        SceHwError SynthChannel::setAsRelative(bool bRelativeToListener)
        {
            m_surroundParams.bListenerRelative = bRelativeToListener;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::setAs2D(bool bIs2d)
        {
            m_surroundParams.bIs2d = bIs2d;
            flagSurroundParamsAsDirty();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SynthChannel::updateParameters()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            float32_t lowpassCutoff = 1.0f;
            float32_t channelPitch = 1.0f;
            float32_t dopplerShift = 1.0f;

            if (m_panner == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = m_panner->calculate(&m_surroundParams, m_numMatrixChannels, m_volumeMatrix, &dopplerShift, &lowpassCutoff);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            for (uint32_t i = 0; i < m_numMatrixChannels; i++)
            {
                m_volumeMatrix[i] *= m_gainMultiplier;
            }

            channelPitch = dopplerShift * m_pitchMultiplier;

            res = setPitch(channelPitch);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = setVolumeMatrix(m_volumeMatrix, m_numMatrixChannels);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = setLowpassCutoff(lowpassCutoff);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_areParamsDirty = false;

            return SCE_HEATWAVE_API_OK;
        }

        void SynthChannel::flagSurroundParamsAsDirty()
        {
            m_areParamsDirty = true;
        }

    } /* HeatWave */
} /* sce */
