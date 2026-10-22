/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_ngs2.h"

#include <libsysmodule.h>
#include <user_service.h>

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
    #include "Synths\heatwave_api_ngs2_a3dmodule.h"
    #pragma comment(lib, "libSceAudio3d_stub_weak.a")
    #define SCE_HEATWAVE_API_PORT_INVALID (SCE_AUDIO3D_PORT_INVALID)
#else
    #define SCE_HEATWAVE_API_PORT_INVALID (-1)
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

/**
* @brief Maximum number of AudioOut ports.
*
* The Audio3d output uses a custom sampler rack, all other destinations use specific master voices
* to feed separate AudioOut ports.
*/
#define NUM_OUTPUT_PORTS (SCE_HEATWAVE_API_OUTPUT_MAX - 1)

#pragma comment(lib, "libSceAudioOut_stub_weak.a")

namespace sce{
    namespace HeatWave{

        /**
        * @brief pass ngs2 allocs into HeatWave (user overrides)
        */
        static int32_t _ngs2bufferAlloc(SceNgs2ContextBufferInfo *bufferInfo)
        {
            if (bufferInfo == NULL)
            {
                return SCE_NGS2_ERROR_EMPTY_BUFFER;
            }

            bufferInfo->hostBuffer = MemoryHandler::mallocCb(bufferInfo->hostBufferSize);
            return (bufferInfo->hostBuffer) ? SCE_OK : SCE_NGS2_ERROR_EMPTY_BUFFER;
        }

        /**
        * @brief pass ngs2 free into HeatWave (user overrides)
        */
        static int32_t _ngs2bufferFree(SceNgs2ContextBufferInfo *bufferInfo)
        {
            if (bufferInfo == NULL)
            {
                return SCE_NGS2_ERROR_EMPTY_BUFFER;
            }

            MemoryHandler::freeCb(bufferInfo->hostBuffer);
            return SCE_OK;
        }

        PlatformSynth* PlatformSynth::createSynth()
        {
            return new Ngs2Synth();
        }

        Ngs2Synth::Ngs2Synth()
            : PlatformSynth(),
            m_ngs2System(SCE_NGS2_HANDLE_INVALID),
            m_ngs2MasterRack(SCE_NGS2_HANDLE_INVALID),
            m_ngs2SamplerRack(SCE_NGS2_HANDLE_INVALID),
            m_a3dSamplerRack(SCE_NGS2_HANDLE_INVALID),
            m_renderThread(0),
            m_isRenderThreadAborted(false),
            m_nSamplesProcessed(0)
        {
            for (int32_t i = 0; i < knMaxNgs2SamplerVoices; i++)
            {
                m_ngs2AvailableVoiceId.push(i);
            }

            for (int32_t i = 0; i < knMaxA3dVoices; i++)
            {
                m_a3dAvailableVoiceId.push(i);
            }

            m_numOutputChannels = kNumChannels;

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_AUDIO3D] = true;
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_AUX] = true;
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_PAD_1] = true;
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_PAD_2] = true;
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_PAD_3] = true;
            m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_PAD_4] = true;
        }

        Ngs2Synth::~Ngs2Synth()
        {
        }

        SceHwError Ngs2Synth::doesContextExist(bool *pOutExists)
        {
            if (pOutExists == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutExists = (m_ngs2System != SCE_NGS2_HANDLE_INVALID);
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2Synth::createContext()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = startNgs2();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (!m_loopback)
            {
                res = startRenderThread();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2Synth::destroyContext()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (!m_loopback)
            {
                res = stopRenderThread();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            res = stopNgs2();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pull a buffer of data from the Synth
        */
        SceHwError Ngs2Synth::renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
        {
            SceNgs2RenderBufferInfo aInfo[1];
            int32_t ngs2Result = SCE_OK;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (uNumSamplesPerChannel == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (uNumChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_ngs2System == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            aInfo[0].buffer         = pBuffer;
            aInfo[0].bufferSize     = uNumSamplesPerChannel * uNumChannels * sizeof(int16_t);
            aInfo[0].numChannels    = uNumChannels;
            aInfo[0].waveformType   = SCE_NGS2_WAVEFORM_TYPE_PCM_I16L;

            ngs2Result = sceNgs2SystemRender(m_ngs2System, aInfo, 1);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_nSamplesProcessed += (int32_t)uNumSamplesPerChannel;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Render a buffer of data for each of the master voices.
        *
        * Render a buffer of data for each of the mastering voices to the corresponding NGS2OutputPort.
        *
        * @param nNumSamplesPerChannel  Number of samples to render for each channel.
        * @param nNumChannels           Number of channels of data to render.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::renderPorts(int32_t nNumSamplesPerChannel, int32_t nNumChannels)
        {
            SceNgs2RenderBufferInfo aInfo[SCE_HEATWAVE_API_OUTPUT_MAX];
            int32_t nRes = SCE_OK;
            int32_t nPortCount = 0;
            int32_t nNumPortChannels = nNumChannels;

            if (nNumSamplesPerChannel <= 0 || nNumChannels <= 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                if (i == SCE_HEATWAVE_API_OUTPUT_AUDIO3D) ///< Audio3d output does not use a separate mastering voice.
                {
                    continue;
                }

                nNumPortChannels = nNumChannels;

                if (i == SCE_HEATWAVE_API_OUTPUT_PAD_1 ||
                    i == SCE_HEATWAVE_API_OUTPUT_PAD_2 ||
                    i == SCE_HEATWAVE_API_OUTPUT_PAD_3 ||
                    i == SCE_HEATWAVE_API_OUTPUT_PAD_4)
                {
                    nNumPortChannels = 1;
                }

                aInfo[nPortCount].buffer        = m_outputs[i].nBuffer;
                aInfo[nPortCount].bufferSize    = nNumPortChannels * nNumSamplesPerChannel * sizeof(int16_t);
                aInfo[nPortCount].numChannels   = nNumPortChannels;
                aInfo[nPortCount].waveformType  = SCE_NGS2_WAVEFORM_TYPE_PCM_I16L;
                nPortCount++;
            }

            nRes = sceNgs2SystemRender(m_ngs2System, aInfo, nPortCount);
            if (nRes < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_nSamplesProcessed += nNumSamplesPerChannel;

            return SCE_HEATWAVE_API_OK;
        }


        SceHwError Ngs2Synth::startNgs2()
        {
            int ngs2Result = SCE_OK;
            SceNgs2BufferAllocator allocator;
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            int32_t nVoiceCount = 0;

            /* load Ngs2 Module */
            ngs2Result = sceSysmoduleLoadModule(SCE_SYSMODULE_NGS2);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            allocator.allocHandler = _ngs2bufferAlloc;
            allocator.freeHandler = _ngs2bufferFree;
            allocator.userData = 0;

            /* create ngs2 system */
            ngs2Result = sceNgs2SystemCreateWithAllocator(NULL, &allocator, &m_ngs2System);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            /* a rack of master voices (1 voice for each output port) */
            nRet = createMasteringRack(NUM_OUTPUT_PORTS, &allocator);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* setup the master voice for each output port */
            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                m_outputs[i].hAudioOutPort = SCE_HEATWAVE_API_PORT_INVALID;
                m_outputs[i].nDestination = i;
                m_outputs[i].hVoiceHandle = SCE_NGS2_HANDLE_INVALID;

                if (i == SCE_HEATWAVE_API_OUTPUT_AUDIO3D) ///< audio3d uses a separate sampler rack, not a port.
                {
                    continue;
                }

                nRet = setupMasteringVoice(nVoiceCount, &m_outputs[i]);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }

                nVoiceCount++;
            }

            /* Connect Audio3d voices to the master voice */
            m_outputs[SCE_HEATWAVE_API_OUTPUT_AUDIO3D].hVoiceHandle = m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN].hVoiceHandle;

            /* rack for sampler voices */
            ngs2Result = createSamplerRack(&allocator);
            if (ngs2Result != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* rack of voices played as libAudio3d objects */
            ngs2Result = createAudio3dSamplerRack(&allocator);
            if (ngs2Result != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2Synth::stopNgs2()
        {
            int32_t ngs2Result = SCE_OK;

            /* stop the master voices and clean up the rack */
            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                if (m_outputs[i].hVoiceHandle != SCE_NGS2_HANDLE_INVALID)
                {
                    ngs2Result = sceNgs2VoiceKickEvent(m_outputs->hVoiceHandle, SCE_NGS2_VOICE_EVENT_KILL);
                    if (ngs2Result < SCE_OK)
                    {
                        return SCE_HEATWAVE_API_ERROR;
                    }
                }
            }
            if (m_ngs2MasterRack != SCE_NGS2_HANDLE_INVALID)
            {
                ngs2Result = sceNgs2RackDestroy(m_ngs2MasterRack, NULL);
                if (ngs2Result < SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                m_ngs2MasterRack = SCE_NGS2_HANDLE_INVALID;
            }

            /* clean up the sampler racks */
            if (m_ngs2SamplerRack != SCE_NGS2_HANDLE_INVALID)
            {
                ngs2Result = sceNgs2RackDestroy(m_ngs2SamplerRack, NULL);
                if (ngs2Result < SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                m_ngs2SamplerRack = SCE_NGS2_HANDLE_INVALID;
            }
            if (m_a3dSamplerRack != SCE_NGS2_HANDLE_INVALID)
            {
                ngs2Result = sceNgs2RackDestroy(m_a3dSamplerRack, NULL);
                if (ngs2Result < SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                m_a3dSamplerRack = SCE_NGS2_HANDLE_INVALID;
            }

            /* shut down the NGS2 system */
            if (m_ngs2System != SCE_NGS2_HANDLE_INVALID)
            {
                ngs2Result = sceNgs2SystemDestroy(m_ngs2System, NULL);
                if (ngs2Result < SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                m_ngs2System = SCE_NGS2_HANDLE_INVALID;
            }

            /* Unload Ngs2 Module */
            ngs2Result = sceSysmoduleUnloadModule(SCE_SYSMODULE_NGS2);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Open the output ports & start a thread to feed audio data from NGS2 to the output ports.
        */
        SceHwError Ngs2Synth::startRenderThread()
        {
            ScePthreadAttr attr;
            int32_t sceRes;
            SceHwError ret = SCE_HEATWAVE_API_OK;

            const int32_t kthreadStackSize = 48 * 1024;

            /* output ports */
            ret = openOutputPorts();
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            /* render thread */
            scePthreadAttrInit(&attr);
            scePthreadAttrSetstacksize(&attr, kthreadStackSize);
            m_isRenderThreadAborted = false;

            sceRes = scePthreadCreate(&m_renderThread, &attr, Ngs2Synth::process, this, "libSceHeatWaveRenderThread");
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            scePthreadAttrDestroy(&attr);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Prepare libAudio3d library for use.
        *
        * Load the Audio3d module and initialise the library.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::initAudio3d()
        {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            int32_t nRet = SCE_OK;

            nRet = sceSysmoduleLoadModule(SCE_SYSMODULE_AUDIO_3D);
            if (nRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            nRet = sceAudio3dInitialize(0);
            if (nRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Release the Audio3d library.
        *
        * Unload the module.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::releaseAudio3d()
        {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            int32_t nSceRes = SCE_OK;

            nSceRes = sceAudio3dTerminate();
            if (nSceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nSceRes = sceSysmoduleUnloadModule(SCE_SYSMODULE_AUDIO_3D);
            if (nSceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
#endif
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Open the various output ports.
        *
        * Open ports for the main output, aux and the pad speakers.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::openOutputPorts()
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            int32_t nSceRes = SCE_OK;

            /* init audioout */
            nSceRes = sceAudioOutInit();
            if (nSceRes < SCE_OK && nSceRes != SCE_AUDIO_OUT_ERROR_ALREADY_INIT)
            {
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            /* init audio3d */
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            nRet = initAudio3d();
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }
#endif
            /* Open main output port */
            nRet = openPort(SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_AUDIO_OUT_PORT_TYPE_MAIN, &m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN]);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* Open aux output port */
            nRet = openPort(SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_AUDIO_OUT_PORT_TYPE_AUX, &m_outputs[SCE_HEATWAVE_API_OUTPUT_AUX]);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Open an output port.
        *
        * Open the libAudioOut\libAudio3d output port.  If libAudio3d is enabled,
        * the corresponding libAudio3d functionality is used to open a port.
        *
        * @param nUser              User ID of the output destination.
        * @param nPortType          Virtual device type.
        * @param pnPortHandleOut    Destination for the created port handle.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::openPort(SceUserServiceUserId nUser, const int32_t nPortType, NGS2OutputPort *pPort)
        {
            int32_t nPortHandle = SCE_HEATWAVE_API_PORT_INVALID;
            uint32_t uFormatParam = SCE_AUDIO_OUT_PARAM_FORMAT_S16_8CH;

            if (pPort == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (nPortType == SCE_AUDIO_OUT_PORT_TYPE_PADSPK)
            {
                uFormatParam = SCE_AUDIO_OUT_PARAM_FORMAT_S16_MONO;
            }

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            /* Open the main audio3d port */
            if (pPort->nDestination == SCE_AUDIO_OUT_PORT_TYPE_MAIN)
            {
                SceAudio3dOpenParameters openParam;
                const uint32_t kuQueueDepth = 3;
                int32_t nSceRes = SCE_OK;
                SceHwMainOutputPortId uA3dPortHandle = SCE_HEATWAVE_API_PORT_INVALID; ///< uint

                sceAudio3dGetDefaultOpenParameters(&openParam);

                openParam.szSizeThis = sizeof(openParam);
                openParam.uiGranularity = kNumSamples;
                openParam.eRate = SCE_AUDIO3D_RATE_48000;
                openParam.uiMaxObjects = knMaxA3dVoices;
                openParam.uiQueueDepth = kuQueueDepth;
                openParam.eBufferMode = SCE_AUDIO3D_BUFFER_ADVANCE_AND_PUSH;

                nSceRes = sceAudio3dPortOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, &openParam, &uA3dPortHandle);
                if (nSceRes != SCE_OK)
                {
                    return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
                }

                nPortHandle = (SceHwAudioOutPortId)uA3dPortHandle; /* cast the uint into an int, later uses then need to cast back. 
                                                       * Use getOutputPortId to access the main port and have it cast to the correct type.
                                                       */
            }
            else
            {
                /* Open an audio out port, through the libAudio3d wrapper */
                nPortHandle = sceAudio3dAudioOutOpen(getOutputPortId(), nUser, nPortType, 0,
                                                        kNumSamples, kSampleRate, uFormatParam);
            }
#else
            /* open a libAudioOut port */
            nPortHandle = sceAudioOutOpen(nUser, nPortType, 0,
                                            kNumSamples, kSampleRate, uFormatParam);
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            if (nPortHandle < SCE_OK)
            {
                if (nPortHandle == SCE_DEVICE_SERVICE_ERROR_USER_NOT_LOGIN)
                {
                    return SCE_HEATWAVE_API_ERROR_PARAM;
                }
                return SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE;
            }

            pPort->hAudioOutPort = nPortHandle;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Release a port handle.
        *
        * Release the previously created port. If libAudio3d is enabled,
        * the corresponding libAudio3d functionality is used.
        *
        * @param pPort Port to release.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::closePort(NGS2OutputPort *pPort)
        {
            int32_t nSceRes = SCE_OK;

            if (pPort == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pPort->hAudioOutPort == SCE_HEATWAVE_API_PORT_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            if (pPort->nDestination == SCE_HEATWAVE_API_OUTPUT_MAIN)
            {
                nSceRes = sceAudio3dPortClose(getOutputPortId());
            }
            else
            {
                nSceRes = sceAudio3dAudioOutClose(pPort->hAudioOutPort);
            }
            pPort->hAudioOutPort = SCE_HEATWAVE_API_PORT_INVALID;
#else
            nSceRes = sceAudioOutOutput(pPort->hAudioOutPort, NULL); ///< Flush the port
            if (nSceRes < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            nSceRes = sceAudioOutClose(pPort->hAudioOutPort);
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */
            if (nSceRes < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief stop the thread feeding data from NGS2 to audioout, shutdown the audioout port
        */
        SceHwError Ngs2Synth::stopRenderThread()
        {
            SceHwError ret = SCE_HEATWAVE_API_OK;

            if (m_renderThread == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* flag the render thread to stop */
            m_isRenderThreadAborted = true;
            scePthreadJoin(m_renderThread, NULL); /* block while the render thread stops */

            ret = closeOutputPorts();
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            ret = releaseAudio3d();
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Close the output ports.
        *
        * Close the main, pad and auxilary output ports.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::closeOutputPorts()
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                if (i != SCE_HEATWAVE_API_OUTPUT_MAIN)
                {
                    if (m_outputs[i].hAudioOutPort != SCE_HEATWAVE_API_PORT_INVALID)
                    {
                        nRet = closePort(&m_outputs[i]);
                        if (SCE_HEATWAVE_API_FAILED(nRet))
                        {
                            return nRet;
                        }
                    }
                }
            }

            if (m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN].hAudioOutPort != SCE_HEATWAVE_API_PORT_INVALID)
            {
                nRet = closePort(&m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN]);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Pull data from NGS2 and feed it to the output port.
        */
        void* Ngs2Synth::process(void *pArg)
        {
            Ngs2Synth   *synth = (Ngs2Synth*)pArg;

            if (synth == NULL)
            {
                return NULL;
            }

            /* Output loop */
            while (synth->killRenderThread() == false)
            {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
                synth->outputlibAudio3d();
#else
                synth->outputAudioOut();
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */
            }

            return NULL;
        }

        /**
        * @brief Render a buffer of audio data to each of the output ports.
        *
        * Render a buffer of data for each NGS2 master voice.
        * Feed the data to the corresponding audioout port.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::outputAudioOut()
        {
            int32_t nPortCount = 0;
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            SceAudioOutOutputParam param[SCE_HEATWAVE_API_OUTPUT_MAX];

            /* get a buffer of audio data for each master voice from NGS2 */
            nRet = renderPorts(Ngs2Synth::kNumSamples, Ngs2Synth::kNumChannels);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* prepare the buffers into an array for audioout */
            for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
            {
                if (i == SCE_HEATWAVE_API_OUTPUT_AUDIO3D) ///< Audio3d output should not contain anything!
                {
                    continue;
                }

                if (m_outputs[i].hAudioOutPort != SCE_HEATWAVE_API_PORT_INVALID)
                {
                    setOutputParam(&m_outputs[i], &param[nPortCount]);
                    nPortCount++;
                }
            }

            /* pass the array of audio buffers */
            sceAudioOutOutputs(param, nPortCount);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Render audio data to each of the output ports.
        *
        * Render a buffer of data for each NGS2 master voice.
        * Feed the data to the corresponding libAudio3d port.
        * Keep processing until the libAudio3d queue is full.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::outputlibAudio3d()
        {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            int32_t nPortCount = 0;
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            SceAudioOutOutputParam param[SCE_HEATWAVE_API_OUTPUT_MAX];
            uint32_t uAvailable = 0;

            sceAudio3dPortGetQueueLevel(getOutputPortId(), NULL, &uAvailable); ///< Amount of space in the queue.

            /* TODO When queuing more than one set of buffers there is stack corruption, needs investigation. */
//             if (uAvailable > 1)
//             {
//                 uAvailable = 1;
//             }

            /* Push audio until we completely fill up the queue */
            for (uint32_t i = 0; i < uAvailable; i++)
            {
                /* Render a buffer of audio data for each master voice */
                nRet = renderPorts(Ngs2Synth::kNumSamples, Ngs2Synth::kNumChannels);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }

                /* prepare the buffers into an array for output */
                for (int32_t i = 0; i < SCE_HEATWAVE_API_OUTPUT_MAX; i++)
                {
                    ///< Main is fed to the bed, audio3d is sent to the main voice.
                    if (i == SCE_HEATWAVE_API_OUTPUT_AUDIO3D || i == SCE_HEATWAVE_API_OUTPUT_MAIN)
                    {
                        continue;
                    }

                    if (m_outputs[i].hAudioOutPort != SCE_HEATWAVE_API_PORT_INVALID)
                    {
                        setOutputParam(&m_outputs[i], &param[nPortCount]);
                        nPortCount++;
                    }
                }

                /* Send buffers to audio out ports (pad, aux) */
                sceAudio3dAudioOutOutputs(param, nPortCount);

                /* Pass the data to the libAudio3d bed */
                sceAudio3dBedWrite(getOutputPortId(), Ngs2Synth::kNumChannels, SCE_AUDIO3D_FORMAT_S16,
                                    &m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN].nBuffer, Ngs2Synth::kNumSamples);

                /* move forward in the queue so the next buffer can be queued. */
                sceAudio3dPortAdvance(getOutputPortId());
            }

            if (uAvailable > 0) ///< Did anything actually get queued?
            {
                /* push our updates, block until their is space in the queue */
                sceAudio3dPortPush(getOutputPortId(), SCE_AUDIO3D_BLOCKING_SYNC);
            }
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Fill out the SceAudioOutputParam correctly for the port.
        *
        * Associate the port and buffer for output.
        *
        * @param pPort The NGS2OutputPort to output data for.
        * @param pParam The parameter structure to fill out.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::setOutputParam(NGS2OutputPort *pPort, SceAudioOutOutputParam *pParam)
        {
            if (pPort == NULL || pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            pParam->handle  = pPort->hAudioOutPort; ///< Port handle.
            pParam->ptr     = pPort->nBuffer;       ///< The buffer of audio data it should output.

            return SCE_HEATWAVE_API_OK;
        }


        /**
        * @brief Has the render thread been flagged to stop
        */
        bool Ngs2Synth::killRenderThread()
        {
            return m_isRenderThreadAborted;
        }

        /**
        * @brief Get an available voice from the sampler rack
        *
        * @param[in]    eDestinationPort    The port the voice should output to.
        * @param[out]   pOutVoiceHandle     The sampler voice handle
        */
        SceHwError Ngs2Synth::getNgs2SamplerVoice(int32_t nDestinationPort, SceNgs2Handle *pOutVoiceHandle)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pOutVoiceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (nDestinationPort == SCE_HEATWAVE_API_OUTPUT_AUDIO3D)
            {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
                res = getSamplerVoiceFromRack(m_a3dSamplerRack, m_a3dAvailableVoiceId, pOutVoiceHandle);
#else
                res = SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */
            }
            else
            {
                res = getSamplerVoiceFromRack(m_ngs2SamplerRack, m_ngs2AvailableVoiceId, pOutVoiceHandle);
            }

            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Setup the master voice rack.
        *
        * We use a seperate master voice for each port that we wish to feed a stream of audio data to.
        * Audio3d objects are an exception, they use a custom NGS2 plugin to feed the audio data to the
        * audio3d object.
        *
        * @param nNumVoices The number of mastering voices to create.
        * @param pAllocator The allocator NGS2 should use.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::createMasteringRack(int32_t nNumVoices, SceNgs2BufferAllocator *pAllocator)
        {
            int32_t nNgsResult = SCE_OK;
            SceNgs2MasteringRackOption masteringOption;

            if (nNumVoices < 0 || pAllocator == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nNgsResult = sceNgs2MasteringRackResetOption(&masteringOption);
            if (nNgsResult < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            masteringOption.rackOption.maxVoices = nNumVoices;

            nNgsResult = sceNgs2RackCreateWithAllocator(m_ngs2System, SCE_NGS2_RACK_ID_MASTERING,
                                                            &masteringOption.rackOption, pAllocator, &m_ngs2MasterRack);
            if (nNgsResult < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Setup the master voice and associate it with the NGS2OutputPort.
        *
        * Set the master voice to output to a separate buffer, start the voice
        * and store a handle to it in the NGS2OutputPort.
        *
        * @param nIndex Rack index of the master voice to use.
        * @param pPort  The port the master voice will belong to.
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::setupMasteringVoice(int32_t nIndex, NGS2OutputPort *pPort)
        {
            SceNgs2Handle hVoiceHandle = SCE_NGS2_HANDLE_INVALID;
            int32_t nRet = SCE_OK;

            if (pPort == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nRet = sceNgs2RackGetVoiceHandle(m_ngs2MasterRack, nIndex, &hVoiceHandle);
            if (nRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = sceNgs2MasteringVoiceSetup(hVoiceHandle, SCE_NGS2_CHANNELS_7_1CH, 0);
            if (nRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            /* set the master voice to output to a seperate buffer when NGS2 renders */
            nRet = sceNgs2MasteringVoiceSetOutput(hVoiceHandle, nIndex);
            if (nRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = sceNgs2VoiceKickEvent(hVoiceHandle, SCE_NGS2_VOICE_EVENT_PLAY);
            if (nRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            pPort->hVoiceHandle = hVoiceHandle;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get an available voice from the supplied sampler rack
        *
        * @param[in]    hRack       The rack to acquire a voice from
        * @param[in]    rIdQueue    Available rack voice indices
        * @param[out]   pHandleOut  The voice
        */
        SceHwError Ngs2Synth::getSamplerVoiceFromRack(SceNgs2Handle hRack, std::queue<int32_t> &rIdQueue, SceNgs2Handle *pHandleOut)
        {
            int32_t voiceIndex = 0;
            int32_t sceRes = SCE_OK;
            SceNgs2Handle voiceHandle = SCE_NGS2_HANDLE_INVALID;

            if (hRack == SCE_NGS2_HANDLE_INVALID || pHandleOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (rIdQueue.size() == 0) /* All voices in the rack are in use */
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            voiceIndex = rIdQueue.front();

            sceRes = sceNgs2RackGetVoiceHandle(hRack, voiceIndex, &voiceHandle);
            if (sceRes != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pHandleOut = voiceHandle;

            rIdQueue.pop(); /* index at ::front() is now in use! */

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Create a rack of NGS2 sampler voices
        */
        int32_t Ngs2Synth::createSamplerRack(SceNgs2BufferAllocator *pAllocator)
        {
            SceNgs2SamplerRackOption samplerRackOption;
            int32_t ret = SCE_OK;

            if (pAllocator == NULL)
            {
                return SCE_NGS2_ERROR_FAIL;
            }

            ret = sceNgs2SamplerRackResetOption(&samplerRackOption);
            if (ret != SCE_OK)
            {
                return ret;
            }

            samplerRackOption.rackOption.maxVoices = knMaxNgs2SamplerVoices;

            ret = sceNgs2RackCreateWithAllocator(m_ngs2System, SCE_NGS2_RACK_ID_SAMPLER, NULL, pAllocator, &m_ngs2SamplerRack);
            if (ret != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_OK;
        }

        /**
        * @brief Create a rack of voices which have a lowpass filter and a user fx.
        * The user fx passes the audio data on to libAudio3d for playback
        */
        int32_t Ngs2Synth::createAudio3dSamplerRack(SceNgs2BufferAllocator *pAllocator)
        {
#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
            SceNgs2CustomSamplerRackOption customSamplerOption;
            SceNgs2CustomUserFx2ModuleOption a3dOption;
            int32_t ret = SCE_OK;

            if (pAllocator == NULL)
            {
                return SCE_NGS2_ERROR_FAIL;
            }

            a3dOption.customModuleOption.size = sizeof(a3dOption);
            a3dOption.workSize = sizeof(A3dModuleWork);
            a3dOption.paramSize = sizeof(A3dModuleParam);
            a3dOption.commonSize = 0;
            a3dOption.userData = (uintptr_t)this;
            a3dOption.processHandler = a3dModuleProcess;
            a3dOption.setupHandler = a3dModuleSetup;
            a3dOption.cleanupHandler = NULL;
            a3dOption.controlHandler = NULL;

            sceNgs2CustomSamplerRackResetOption(&customSamplerOption);
            customSamplerOption.customRackOption.aModule[0].moduleId = SCE_NGS2_CUSTOM_MODULE_ID_FILTER;
            customSamplerOption.customRackOption.aModule[0].option = NULL;

            customSamplerOption.customRackOption.aModule[1].moduleId = SCE_NGS2_CUSTOM_MODULE_ID_USER_FX2;
            customSamplerOption.customRackOption.aModule[1].stateOffset = 0;
            customSamplerOption.customRackOption.aModule[1].stateSize = 0;
            customSamplerOption.customRackOption.aModule[1].option = &a3dOption.customModuleOption;

            customSamplerOption.customRackOption.numModules = 2;
            customSamplerOption.customRackOption.stateSize = sizeof(SceNgs2CustomSamplerVoiceState);

            customSamplerOption.maxAtrac9Decoders = knMaxA3dVoices;
            customSamplerOption.customRackOption.rackOption.maxVoices = knMaxA3dVoices;

            ret = sceNgs2RackCreateWithAllocator(m_ngs2System, SCE_NGS2_RACK_ID_CUSTOM_SAMPLER, &customSamplerOption.customRackOption.rackOption, pAllocator, &m_a3dSamplerRack);
            if (ret != SCE_OK)
            {
                return ret;
            }
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

            return SCE_OK;
        }

        /**
        * @brief Flag sampler voice as available for re-use.
        */
        SceHwError Ngs2Synth::releaseNgs2Voice(SceNgs2Handle hVoiceHandle, int32_t nDestinationPort)
        {
            int32_t nSceRet = SCE_OK;
            SceNgs2Handle nRackHandle = SCE_NGS2_HANDLE_INVALID;
            uint32_t uVoiceId = 0;

            if (hVoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nSceRet = sceNgs2VoiceGetOwner(hVoiceHandle, &nRackHandle, &uVoiceId);
            if (nSceRet != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nSceRet = sceNgs2VoiceKickEvent(hVoiceHandle, SCE_NGS2_VOICE_EVENT_KILL);
            if (nSceRet < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (nDestinationPort == SCE_HEATWAVE_API_OUTPUT_AUDIO3D)
            {
                m_a3dAvailableVoiceId.push(uVoiceId);
            }
            else
            {
                m_ngs2AvailableVoiceId.push(uVoiceId);
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Connect the sampler voice hVoiceHandle to the master voice
        */
        SceHwError Ngs2Synth::patchNgs2VoiceToDestination(SceNgs2Handle hVoiceHandle, int32_t nDestinationPort)
        {
            int32_t ngs2Result = SCE_OK;
            const SceNgs2Handle hDestinationMasterVoice = m_outputs[nDestinationPort].hVoiceHandle;

            if (hVoiceHandle == SCE_NGS2_HANDLE_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ngs2Result = sceNgs2VoicePatch(hVoiceHandle, 0, hDestinationMasterVoice, 0);
            if (ngs2Result < SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Increment the count of active 3dAudio objects
        *
        * @note Temporary until callbacks are implemented
        */
        void Ngs2Synth::IncAudio3dObjectCount()
        {
            m_numAudio3dObjects++;
        }

        /**
        * @brief Decrement the count of active 3dAudio objects
        *
        * @note Temporary until callbacks are implemented
        */
        void Ngs2Synth::DecAudio3dObjectCount()
        {
            m_numAudio3dObjects--;
        }

        SceHwMainOutputPortId Ngs2Synth::getOutputPortId()
        {
            return m_outputs[SCE_HEATWAVE_API_OUTPUT_MAIN].hAudioOutPort;
        }

        SceHwError Ngs2Synth::openUserPort(int32_t nPortId, int32_t nUserId)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            if (nPortId < SCE_HEATWAVE_API_OUTPUT_PAD_1 || nPortId > SCE_HEATWAVE_API_OUTPUT_PAD_4)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_outputs[nPortId].hAudioOutPort != SCE_HEATWAVE_API_PORT_INVALID)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION; ///< Port is already open.
            }

            nRet = openPort(nUserId, SCE_AUDIO_OUT_PORT_TYPE_PADSPK, &m_outputs[nPortId]);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError Ngs2Synth::closeUserPort(int32_t nPortId)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            if (nPortId < SCE_HEATWAVE_API_OUTPUT_PAD_1 || nPortId > SCE_HEATWAVE_API_OUTPUT_PAD_4)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nRet = closePort(&m_outputs[nPortId]);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Retrieve information about the synth.
        *
        * @param[out] pInfoOut  Structure to fill out with information about the synth.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Ngs2Synth::getInfo(PlatformSynthInfo *pInfoOut) const
        {
            if (pInfoOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            pInfoOut->nSampleRate = kSampleRate;
            pInfoOut->nSamplesElapsed = m_nSamplesProcessed;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
