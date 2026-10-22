/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "Synths\heatwave_api_synthchannel_ngs2libaudio3d.h"

#include <sceerror.h>
#include "heatwave_api_panner.h"
#include "Synths\heatwave_api_synth_ngs2.h"
#include "Synths\heatwave_api_ngs2_a3dmodule.h"
#include <math.h>

namespace sce{
    namespace HeatWave{

        NGS2LibAudio3dSynthChannel::NGS2LibAudio3dSynthChannel()
            : Ngs2SynthChannel()
        {
        }

        NGS2LibAudio3dSynthChannel::~NGS2LibAudio3dSynthChannel()
        {
        }

        /**
        * @brief NGS2 Custom Voice specific setup
        */
        SceHwError NGS2LibAudio3dSynthChannel::setupNgs2Voice(SceNgs2Handle hVoiceHandle, SceNgs2WaveformFormat *pWaveFormat, uint32_t uFlags)
        {
            int32_t nRet = SCE_OK;
            const float32_t kfNgs2MsaterVoiceSend = 0.0f;
            const uint32_t kuMasterVoicePort = 0;

            if (hVoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pWaveFormat == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nRet = sceNgs2CustomSamplerVoiceSetup(hVoiceHandle, pWaveFormat, uFlags);
            if (nRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = sceNgs2VoiceSetPortVolume(hVoiceHandle, kuMasterVoicePort, kfNgs2MsaterVoiceSend); /* Alter the gain of audio sent to the NGS2 master voice as it will also be handled by Audio3d */
            if (nRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Custom Sampler voice pitch set
        */
        SceHwError NGS2LibAudio3dSynthChannel::setPitch(float32_t fPitch)
        {
            int32_t ngs2Result = SCE_OK;

            if (m_ngs2VoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2CustomSamplerVoiceSetPitch(m_ngs2VoiceHandle, fPitch);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Custom Sampler voice specific filter set
        */
        SceHwError NGS2LibAudio3dSynthChannel::setNgs2VoiceFilterByFcq(SceNgs2Handle hVoiceHandle, uint32_t uIndex, uint32_t uLocation, uint32_t uType, uint32_t uChannelMask, float fC, float fQ, float fLevel)
        {
            int32_t ret = SCE_OK;

            ret = sceNgs2CustomVoiceSetFilterByFcq(hVoiceHandle, uIndex, uType, uChannelMask, fC, fQ, fLevel);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Custom Sampler voice specific waveform block add
        */
        SceHwError NGS2LibAudio3dSynthChannel::addNgs2VoiceWaveformBlocks(SceNgs2Handle hVoiceHandle, const void *pData, const SceNgs2WaveformBlock *pBlock, uint32_t uNumBlocks, uint32_t uFlags)
        {
            int32_t ret = SCE_OK;

            ret = sceNgs2CustomSamplerVoiceAddWaveformBlocks(hVoiceHandle, pData, pBlock, uNumBlocks, uFlags);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief NGS2 Sampler voice specific query decoded data size
        */
        SceHwError NGS2LibAudio3dSynthChannel::getNgs2VoiceDecodedDataSize(SceNgs2Handle hVoiceHandle, uint64_t *pDecodedDataSizeOut)
        {
            int32_t ret = SCE_OK;
            SceNgs2CustomSamplerVoiceState voiceState;

            MemoryHandler::memsetCb(&voiceState, 0, sizeof(SceNgs2SamplerVoiceState));

            ret = sceNgs2CustomSamplerVoiceGetState(hVoiceHandle, &voiceState);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pDecodedDataSizeOut = voiceState.decodedDataSize;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate attenuation, doppler and  cone effects.  Pass on the position to libAudio3d for speaker
        * volume calculations.
        *
        * @note SCE_HEATWAVE_API_USE_LIBAUDIO3D must be defined for this method to execute
        */
        SceHwError NGS2LibAudio3dSynthChannel::updateParameters()
        {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            SceHwError ret = SCE_HEATWAVE_API_OK;
            int32_t sceRes = SCE_OK;
            A3dModuleParam a3dParam;
            float32_t dopplerShift = 1.0f;
            float32_t distanceAttenuation = 1.0f;
            float32_t coneAttenuation = 1.0f;
            float32_t coneLowpass = 1.0f;
            float32_t channelPitch = 1.0f;

            const float32_t kfSpreadMax = 2.0f * M_PI;
            const float32_t kfDefaultSpread = kfSpreadMax / 2.0f;

            a3dParam.nPriority = m_surroundParams.nPriority;
            a3dParam.vPosition = { 0.0f, 0.0f, 0.0f };
            a3dParam.fSpread = kfDefaultSpread;
            a3dParam.fGain = 1.0f;

            ret = m_panner->getListenerRelativePosition(m_surroundParams.vPosition, &a3dParam.vPosition);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            ret = m_panner->calcDistanceGain(a3dParam.vPosition, m_surroundParams.fMinDistance, m_surroundParams.fMaxDistance, &distanceAttenuation);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            ret = m_panner->calcCone(a3dParam.vPosition, m_surroundParams.vForward, m_surroundParams.fInsideAngle, m_surroundParams.fOutsideAngle, m_surroundParams.fOutsideGain, m_surroundParams.fOutsideFreq, &coneAttenuation, &coneLowpass);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            ret = m_panner->calcDopplerShift(a3dParam.vPosition, m_surroundParams.vVelocity, &dopplerShift);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            channelPitch = dopplerShift * m_pitchMultiplier;
            a3dParam.fGain *= distanceAttenuation * m_gainMultiplier;

            /* Pass the parameters to the libAudio3d object */
            sceRes = sceNgs2CustomVoiceSetUserFx2(m_ngs2VoiceHandle, 1, &a3dParam, sizeof(a3dParam));
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = setPitch(channelPitch);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            ret = setLowpassCutoff(coneLowpass);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            m_areParamsDirty = false;
#else
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Disable the channel volume matrix, libAudio3d will perform the final channel mix down
        */
        SceHwError NGS2LibAudio3dSynthChannel::setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels)
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
        }

    } /* HeatWave */
} /* sce */
