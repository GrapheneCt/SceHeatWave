/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_xaudio2.h"
#include <Windows.h>

namespace sce{
    namespace HeatWave{

        PlatformSynth* PlatformSynth::createSynth()
        {
            return new XAudio2Synth();
        }

        XAudio2Synth::XAudio2Synth()
            : PlatformSynth(),
            m_xaudio2Engine(NULL),
            m_xaudio2MasterVoice(NULL)
        {
        }

        XAudio2Synth::~XAudio2Synth()
        {
        }

        SceHwError XAudio2Synth::doesContextExist(bool *pOutExists)
        {
            if (pOutExists == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutExists = ((m_xaudio2Engine != NULL) && (m_xaudio2MasterVoice != NULL));
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get an IXAudio2SourceVoice that feeds data to the IXAudio2MasteringVoice
        */
        SceHwError XAudio2Synth::getXAudio2Voice(const WAVEFORMATEX *pXAudio2Format, IXAudio2SourceVoice **pXAudio2VoiceOut)
        {
            HRESULT xaudio2Result = S_OK;

            if (pXAudio2Format == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pXAudio2VoiceOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (m_xaudio2Engine == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            xaudio2Result = m_xaudio2Engine->CreateSourceVoice(pXAudio2VoiceOut, pXAudio2Format, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get the IXaudio2MasteringVoice handle
        */
        SceHwError XAudio2Synth::getXAudio2MasterVoice(IXAudio2MasteringVoice **pXAudio2MasterVoiceOut)
        {
            if (pXAudio2MasterVoiceOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_xaudio2Engine == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }
            if (m_xaudio2MasterVoice == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            *pXAudio2MasterVoiceOut = m_xaudio2MasterVoice;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2Synth::createContext()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            HRESULT xaudio2Result = S_OK;

            if (m_loopback) 
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION; /* No simple way to support loopback in XAudio2 */
            }

            xaudio2Result = CoInitializeEx(NULL, COINIT_MULTITHREADED); /* start up XAudio2 */
            if (FAILED(xaudio2Result))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            res = createXAudio2Engine();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = createXAudio2MasterVoice();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2Synth::destroyContext()
        {
            if (m_xaudio2MasterVoice != NULL)
            {
                m_xaudio2MasterVoice->DestroyVoice();
            }

            if (m_xaudio2Engine != NULL)
            {
                m_xaudio2Engine->Release();
            }

            CoUninitialize();

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError XAudio2Synth::renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION; /* No loopback support in XAudio2 */
        }

        SceHwError XAudio2Synth::createXAudio2Engine()
        {
            uint32_t    xaudio2InitFlags = 0;
            HRESULT     xaudio2Result = S_OK;

            xaudio2Result = XAudio2Create(&m_xaudio2Engine, xaudio2InitFlags);
            if (FAILED(xaudio2Result))
            {
                CoUninitialize();
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }
            if (m_xaudio2Engine == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Create the IXAudio2MasteringVoice
        */
        SceHwError XAudio2Synth::createXAudio2MasterVoice()
        {
            const int32_t knNumOutputChannels = 8; /* 7.1 let XAudio2 deal with the final format folddown */
            HRESULT xaudio2Result = S_OK;
            XAUDIO2_VOICE_DETAILS xaudio2MasterVoiceDetails = {0};

            if (m_xaudio2Engine == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            /* setup the master voice */
            xaudio2Result = m_xaudio2Engine->CreateMasteringVoice(&m_xaudio2MasterVoice, knNumOutputChannels);
            if (FAILED(xaudio2Result))
            {
                m_xaudio2Engine->Release();
                CoUninitialize();

                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            m_xaudio2MasterVoice->GetVoiceDetails(&xaudio2MasterVoiceDetails);

            m_numOutputChannels = xaudio2MasterVoiceDetails.InputChannels;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
