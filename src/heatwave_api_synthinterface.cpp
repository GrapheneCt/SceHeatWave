/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_synthinterface.h"

/**
* @brief Generate a compile error if compiling with Audio3d functionality on unsupported platforms
*/
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
    #ifndef __ORBIS__
        #error "Unsupported platform - libAudio3d unavailable" ///< libAudio3d functionality is only supported on PS4
    #endif
#endif

namespace sce{
    namespace HeatWave{

        PlatformSynth::PlatformSynth()
            : m_isInitialised(false),
            m_doesHeatWaveOwnContext(false),
            m_loopback(false),
            m_numOutputChannels(0),
            m_numAudio3dObjects(0)
        {
            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                m_bPortSupported[i] = false;
            }
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_MAIN] = true; ///< All synths have to output somewhere!
        }

        SceHwError PlatformSynth::init(bool bLoopBack)
        {
            bool contextExists = false;
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (m_isInitialised)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION; /* Synth is already setup */
            }

            m_isInitialised = true;
            m_loopback = bLoopBack;

            /* Create a hardware context if a context does not already exist */
            res = doesContextExist(&contextExists);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (!contextExists)
            {
                m_doesHeatWaveOwnContext = true;

                res = createContext();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError PlatformSynth::release()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            bool contextExists = false;

            if (!m_isInitialised)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            res = doesContextExist(&contextExists);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (!contextExists)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            if (m_doesHeatWaveOwnContext) /* Clean up the hardware context only when HeatWave has ownership */
            {
                res = destroyContext();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError PlatformSynth::createChannel(SynthChannel::InitParams *pParam, SynthChannel **pOutChannel)
        {
            bool contextExists = false;
            SynthChannel *newChannel = NULL;
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (pParam->uNumChannels == 0 || pParam->uNumChannels > SynthChannel::kuMaxChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParam->uSampleRate == 0 || pParam->uSampleRate > SynthChannel::kuMaxSampleRate)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pOutChannel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = doesContextExist(&contextExists);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (!contextExists)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            if (!isPortSupported(pParam->nDestinationPort))
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            /* Disable anything that does not output through the main output.  
            The API will either need updating to support loopback of the different output destinations or all destinations should output to
            the loopback buffer. For now this is the clearest solution, as it actually returns something to the user.
            */
            if (m_loopback == true && pParam->nDestinationPort != SCE_HEATWAVE_API_OUTPUT_MAIN)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            newChannel = SynthChannel::createChannel(pParam->nDestinationPort);
            if (newChannel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            res = newChannel->init(this, pParam);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                newChannel->release();
                delete newChannel;
                return res;
            }

            *pOutChannel = newChannel;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError PlatformSynth::destroyChannel(SynthChannel *pChannel)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pChannel == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = pChannel->release();
            delete pChannel;

            return res;
        }

        /**
        * @brief render into pBuffer (16bit PCM)
        *
        * @return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION if m_loopback == false
        */
        SceHwError PlatformSynth::render(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
        {
            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (!m_loopback)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            return renderContext(pBuffer, uNumSamplesPerChannel, uNumChannels);
        }

        SceHwError PlatformSynth::isLoopback(bool *pbIsInLoopbackMode) const
        {
            if (pbIsInLoopbackMode == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pbIsInLoopbackMode = m_loopback;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError PlatformSynth::getNumOutputChannels(uint32_t *pNumChannels)
        {
            if (pNumChannels == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_numOutputChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            *pNumChannels = m_numOutputChannels;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get the count of currently active libAudio3d objects
        */
        SceHwError PlatformSynth::getAudio3dObjectCount(uint32_t *pObjectCountOut)
        {
            if (pObjectCountOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pObjectCountOut = m_numAudio3dObjects;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Does the synth support output to the port type?
        *
        * Query the synth for support to output to a specific output port.
        * In loopback only the main port is supported (the HeatWave api currently has no
        * mechanism to render the outputs for the separate output ports).
        *
        * @param[in] ePort  The Port to query.
        * @return true if the PlatformSynth supports output to the port.
        */
        bool PlatformSynth::isPortSupported(int32_t nPort)
        {
            if (m_loopback == true)
            {
                return nPort == SCE_HEATWAVE_API_OUTPUT_MAIN;
            }
            return m_bPortSupported[nPort];
        }

        SceHwError PlatformSynth::openUserPort(int32_t nPortId, int32_t nUserId)
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
        }

        SceHwError PlatformSynth::closeUserPort(int32_t nPortId)
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
        }

        /**
        * @brief Retrieve information about the synth.
        *
        * Dummy implementation.  Synths should override this function, default behviour
        * is to return an error code indicating that the function is not supported.
        *
        * @param[out] pInfoOut  Structure to fill out with information about the synth.
        *
        * @return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION
        */
        SceHwError PlatformSynth::getInfo(PlatformSynthInfo *pInfoOut) const
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
        }

    } /* sce */
} /* HeatWave */
