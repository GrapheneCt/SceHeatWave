/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_ngs.h"
#include <kernel.h>
#include <audioout.h>
#include <string.h>
#include "heatwave_api_systeminternal.h"

namespace sce{
    namespace HeatWave{
        /**
        * @brief pass ngs allocs into HeatWave (user overrides)
        */
        void *_ngsMemAlign(uint32_t nSize, uint32_t nAlign)
        {
            return MemoryHandler::memalignCb(nAlign, nSize + nAlign);
        }

        /**
        * @brief pass ngs allocs into HeatWave (user overrides)
        */
        void *_ngsMalloc(uint32_t nSize)
        {
            return MemoryHandler::mallocCb(nSize);
        }

        /**
        * @brief pass ngs free into HeatWave (user overrides)
        */
        void _ngsFree(void *pPtr)
        {
            MemoryHandler::freeCb(pPtr);
        }
        
        PlatformSynth* PlatformSynth::createSynth()
        {
            PlatformSynth *pReturn = new NgsSynth();
            return pReturn;
        }

        NgsSynth::NgsSynth()
            : PlatformSynth(),
            m_pSysMem(NULL),
            m_pRackMemMaster(NULL),
            m_pRackMemVoiceMono(NULL),
            m_pRackMemVoiceStereo(NULL),
            m_NGSSysHandle(NULL),
            m_RackMaster(NULL),
            m_RackVoiceMono(NULL),
            m_RackVoiceStereo(NULL),
            m_ThreadIdAudioUpdate(NULL),
            m_nAudioOutPortId(NULL),
            m_bIsAudioOutAborted(false),
            nSamplePosition(kNumSamples * kMaxChannels),
            pOutputMemory(NULL),
            m_nSamplesProcessed(0)
        {
            memset(&m_pVoiceUsed, 0, sizeof(bool) * kMaxVoices * 2);

            m_numOutputChannels = 2;
        }

        NgsSynth::~NgsSynth()
        {
            bool bNeedsRelease;
            doesContextExist(&bNeedsRelease);

            if (bNeedsRelease)
            {
                destroyContext();
            }
        }

        SceHwError NgsSynth::doesContextExist(bool *pOutExists)
        {
            if (pOutExists == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutExists = !(m_NGSSysHandle == NULL);

            return SCE_HEATWAVE_API_OK;
        }


        int NgsSynth::connectRacks( SceNgsHVoice hVoiceSource, SceNgsHVoice hVoiceDest, int nIndex )
        {
            int                  returnCode;
            SceNgsPatchSetupInfo patchInfo;
            SceNgsPatchRouteInfo patchRouteInfo;

            // Create patch
            patchInfo.hVoiceSource           = hVoiceSource;
            patchInfo.nSourceOutputIndex     = 0;
            patchInfo.nSourceOutputSubIndex  = SCE_NGS_VOICE_PATCH_AUTO_SUBINDEX;
            patchInfo.hVoiceDestination      = hVoiceDest;
            patchInfo.nTargetInputIndex      = 0;

            returnCode = sceNgsPatchCreateRouting( &patchInfo, &patch[nIndex] );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            // Set volumes on patch
            returnCode = sceNgsPatchGetInfo( patch[nIndex], &patchRouteInfo, NULL );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }
            if ( patchRouteInfo.nOutputChannels == 1 ) {
                if ( patchRouteInfo.nInputChannels == 1 ) {
                    patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
                } else {
                    patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
                    patchRouteInfo.vols.m[0][1] = 1.0f; // left to right
                }
            } else {
                if ( patchRouteInfo.nInputChannels == 1 ) {
                    patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
                    patchRouteInfo.vols.m[1][0] = 1.0f; // right to left
                } else {
                    patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
                    patchRouteInfo.vols.m[0][1] = 0.0f; // left to right
                    patchRouteInfo.vols.m[1][0] = 0.0f; // right to left
                    patchRouteInfo.vols.m[1][1] = 1.0f; // right to right
                }
            }
            returnCode = sceNgsVoicePatchSetVolumesMatrix( patch[nIndex], &patchRouteInfo.vols );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return returnCode;
            }

            return SCE_OK;
        }

        SceHwError NgsSynth::startNgs()
        {
            int nCnt;
            const int32_t nMaxModules = 8;
            SceNgsSystemInitParams InitParams;
            int nReturnCode;
            size_t SizeRequired;

            const struct SceNgsVoiceDefinition *pMasterBussVoiceDef;
            SceNgsRackDescription               rackDesc;
            SceNgsBufferInfo                    bufferInfo;
            const struct SceNgsVoiceDefinition *pSimpleVoiceTemplate;

            // initialize NGS lib
            nReturnCode = sceSysmoduleLoadModule( SCE_SYSMODULE_NGS );
            if ( nReturnCode != SCE_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            InitParams.nMaxRacks    = 3; //voices mono + voices stereo + master
            InitParams.nMaxVoices   = kMaxVoices + kMaxVoices + 1; //voices + master
            InitParams.nGranularity = kNumSamples;
            InitParams.nSampleRate  = kSampleRate;
            InitParams.nMaxModules  = nMaxModules;

            // Determine memory requirement
            nReturnCode = sceNgsSystemGetRequiredMemorySize( &InitParams, &SizeRequired );
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            // Allocate aligned memory
            m_pSysMem = _ngsMemAlign( SizeRequired, SCE_NGS_MEMORY_ALIGN_SIZE );
            if ( m_pSysMem == NULL ) 
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            // Initialise NGS and get system handle
            nReturnCode = sceNgsSystemInit( m_pSysMem, SizeRequired, &InitParams, &m_NGSSysHandle );
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            //Create master rack
            pMasterBussVoiceDef = sceNgsVoiceDefGetMasterBuss();

            rackDesc.nChannelsPerVoice      = 2;
            rackDesc.nVoices                = 1;
            rackDesc.pVoiceDefn             = pMasterBussVoiceDef;
            rackDesc.nMaxPatchesPerInput    = kMaxVoices + kMaxVoices;
            rackDesc.nPatchesPerOutput      = 0;

            nReturnCode = sceNgsRackGetRequiredMemorySize( m_NGSSysHandle, &rackDesc, &bufferInfo.size);
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            // Allocate memory for master rack
            m_pRackMemMaster = _ngsMemAlign( bufferInfo.size, SCE_NGS_MEMORY_ALIGN_SIZE );
            if ( m_pRackMemMaster == NULL ) 
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            memset( m_pRackMemMaster, 0, bufferInfo.size );

            // Initialise master rack
            bufferInfo.data = m_pRackMemMaster;

            nReturnCode = sceNgsRackInit( m_NGSSysHandle, &bufferInfo, &rackDesc, &m_RackMaster );
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nReturnCode = sceNgsRackGetVoiceHandle(m_RackMaster, 0, &MasterVoice);
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            //Create mono channel rack
            pSimpleVoiceTemplate = sceNgsVoiceDefGetSimpleVoice();

            // Determine memory requirement for player rack
            rackDesc.nChannelsPerVoice   = 1;
            rackDesc.nVoices             = kMaxVoices;
            rackDesc.pVoiceDefn          = pSimpleVoiceTemplate;
            rackDesc.nMaxPatchesPerInput = 0;
            rackDesc.nPatchesPerOutput   = 1;

            nReturnCode = sceNgsRackGetRequiredMemorySize( m_NGSSysHandle, &rackDesc, &bufferInfo.size);
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pRackMemVoiceMono = _ngsMemAlign( bufferInfo.size , SCE_NGS_MEMORY_ALIGN_SIZE );
            if ( m_pRackMemVoiceMono == NULL ) 
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            memset( m_pRackMemVoiceMono, 0, bufferInfo.size );
            bufferInfo.data = m_pRackMemVoiceMono;

            // Initialise player rack
            bufferInfo.data = m_pRackMemVoiceMono;
            nReturnCode = sceNgsRackInit( m_NGSSysHandle, &bufferInfo, &rackDesc, &m_RackVoiceMono );
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            //Create stereo channel rack
            // Determine memory requirement for player rack
            rackDesc.nChannelsPerVoice   = 2;
            rackDesc.nVoices             = kMaxVoices;
            rackDesc.pVoiceDefn          = pSimpleVoiceTemplate;
            rackDesc.nMaxPatchesPerInput = 0;
            rackDesc.nPatchesPerOutput   = 1;

            nReturnCode = sceNgsRackGetRequiredMemorySize( m_NGSSysHandle, &rackDesc, &bufferInfo.size);
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_pRackMemVoiceStereo = _ngsMemAlign( bufferInfo.size, SCE_NGS_MEMORY_ALIGN_SIZE );
            if ( m_pRackMemVoiceStereo == NULL ) 
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            memset( m_pRackMemVoiceStereo, 0, bufferInfo.size  + (SCE_NGS_MEMORY_ALIGN_SIZE - 1));
            bufferInfo.data = (void *)((int)m_pRackMemVoiceStereo & ~(SCE_NGS_MEMORY_ALIGN_SIZE - 1));

            // Initialise player rack
            bufferInfo.data = m_pRackMemVoiceStereo;
            nReturnCode = sceNgsRackInit( m_NGSSysHandle, &bufferInfo, &rackDesc, &m_RackVoiceStereo );
            if ( nReturnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            //connect player racks to master
            for (nCnt = 0; nCnt < kMaxVoices; nCnt++)
            {
                SceNgsHVoice ChannelVoiceMono;
                SceNgsHVoice ChannelVoiceStereo;
                nReturnCode = sceNgsRackGetVoiceHandle(m_RackVoiceMono, nCnt, &ChannelVoiceMono);
                if ( nReturnCode != SCE_NGS_OK ) 
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                nReturnCode = sceNgsRackGetVoiceHandle(m_RackVoiceStereo, nCnt, &ChannelVoiceStereo);
                if ( nReturnCode != SCE_NGS_OK ) 
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                nReturnCode = connectRacks(ChannelVoiceMono, MasterVoice, nCnt);
                if ( nReturnCode != SCE_NGS_OK ) 
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                nReturnCode = connectRacks(ChannelVoiceStereo, MasterVoice, nCnt + kMaxVoices);
                if ( nReturnCode != SCE_NGS_OK ) 
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            nReturnCode = sceNgsVoicePlay( MasterVoice );

            return SCE_HEATWAVE_API_OK;
        }

        SceNgsHPatch NgsSynth::GetPatch(int nIndex)
        {
            if ((nIndex < 0) || (nIndex >= kMaxVoices * 2))
            {
                return SCE_NGS_INVALID_HANDLE;
            }

            return patch[nIndex];
        }

        SceHwError NgsSynth::stopNgs()
        {
            int returnCode;

            returnCode = sceNgsSystemRelease( m_NGSSysHandle );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            sceSysmoduleUnloadModule( SCE_SYSMODULE_NGS );

            _ngsFree(m_pSysMem);
            _ngsFree(m_pRackMemMaster);
            _ngsFree(m_pRackMemVoiceMono);
            _ngsFree(m_pRackMemVoiceStereo);
            m_NGSSysHandle = NULL;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynth::stopAudioOutThread()
        {
            int returnCode;
            int n32ThreadExitStatus;

            // Stop audio update thread
            m_bIsAudioOutAborted = true;

            returnCode = sceKernelWaitThreadEnd( m_ThreadIdAudioUpdate, &n32ThreadExitStatus, SCE_NULL );
            if ( returnCode != SCE_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            
            return stopAudioOut();
        }

        SceHwError NgsSynth::stopAudioOut()
        {
            int returnCode;

            returnCode = sceAudioOutOutput( m_nAudioOutPortId, NULL );
            if ( returnCode < 0 ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            returnCode = sceAudioOutReleasePort( m_nAudioOutPortId );
            if ( returnCode < 0 ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Increase the number of samples processed by the synth.
        *
        * @param[in] nNumSamples The number of samples to add the the count of processed samples.
        */
        void NgsSynth::incSamplesProcessed(int32_t nNumSamples)
        {
            m_nSamplesProcessed += nNumSamples;
        }

        bool NgsSynth::updateThreadIsRunning()
        {
            return !m_bIsAudioOutAborted;
        }

        int32_t NgsSynth::getAudioOutPortId()
        {
            return m_nAudioOutPortId;
        }

        SceHwError NgsSynth::process(short *pOutput)
        {
            int   returnCode     = SCE_OK;

            if (pOutput == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            // Perform NGS system update
            if (SystemInternal::getInstance() != NULL && SystemInternal::getInstance()->isInitialised())
            {
                SCE_HEATWAVE_LOCK();
                returnCode = sceNgsSystemUpdate(m_NGSSysHandle);
                SCE_HEATWAVE_UNLOCK();

                if (returnCode != SCE_NGS_OK)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
            }

            // Get output data
            returnCode = sceNgsVoiceGetStateData( MasterVoice, SCE_NGS_MASTER_BUSS_OUTPUT_MODULE, pOutput, sizeof(short) * kNumSamples * 2 );
            if ( returnCode != SCE_NGS_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        int AudioUpdateThread(SceSize args, void *argc)
        {
            int   returnCode     = SCE_OK;
            int   nCurrentBuffer = 0;
            short  s_ptrOutputBuffer[2][NgsSynth::kNumSamples * 2];

            NgsSynth *pSynth = *((NgsSynth **)argc);

            while ( pSynth->updateThreadIsRunning() ) 
            {
                pSynth->process(&s_ptrOutputBuffer[nCurrentBuffer][0]);

                // Output audio to device and/or file
                returnCode = sceAudioOutOutput( pSynth->getAudioOutPortId(),  &s_ptrOutputBuffer[nCurrentBuffer][0] );
                if ( returnCode < 0 ) 
                {
                    return returnCode;
                }

                // Switch buffer
                nCurrentBuffer ^= 1;

                pSynth->incSamplesProcessed(NgsSynth::kNumSamples);
            }

            //Flush last buffer through audio output
            returnCode = sceAudioOutOutput( pSynth->getAudioOutPortId(),  NULL );
            if ( returnCode < 0 ) 
            {
                return returnCode;
            }

            sceKernelExitDeleteThread( returnCode );

            return returnCode;
        }

        SceHwError NgsSynth::startAudioOut()
        {
            int nVolume[2] = { SCE_AUDIO_VOLUME_0dB, SCE_AUDIO_VOLUME_0dB };
            int nReturn;

            m_nAudioOutPortId = sceAudioOutOpenPort( SCE_AUDIO_OUT_PORT_TYPE_MAIN, kNumSamples, kSampleRate, SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);    

            if ( m_nAudioOutPortId < 0 ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nReturn = sceAudioOutSetVolume( m_nAudioOutPortId, (SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), nVolume );
            if (nReturn != SCE_OK)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_OK;
        }


        SceHwError NgsSynth::startAudioOutThread()
        {
            int nReturn;
            const int32_t nAudioStackSize = 32 * 1024;
            void *pArg;

            m_bIsAudioOutAborted = false;

            nReturn = startAudioOut();
            if ( nReturn != SCE_OK ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_ThreadIdAudioUpdate = sceKernelCreateThread(  "NGS Audio Update Thread", AudioUpdateThread, SCE_KERNEL_HIGHEST_PRIORITY_USER, nAudioStackSize, 0, SCE_KERNEL_CPU_MASK_USER_ALL, SCE_NULL );
            if ( m_ThreadIdAudioUpdate < 0 ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            pArg = this;
            nReturn = sceKernelStartThread( m_ThreadIdAudioUpdate, sizeof(void*), &pArg );
            if ( nReturn < 0 ) 
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }


        SceHwError NgsSynth::createContext()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = startNgs();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (pOutputMemory != NULL) //we have left over memory, best not leak
            {
                free(pOutputMemory);
            }

            pOutputMemory = (int16_t *)_ngsMalloc(kMaxChannels * kNumSamples * sizeof(int16_t));
            if (pOutputMemory == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            if (!m_loopback)
            {
                res = startAudioOutThread();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynth::destroyContext()
        {
            int res;

            if (!m_loopback)
            {
                res = stopAudioOutThread();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            res = stopNgs();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (pOutputMemory != NULL)
            {
                _ngsFree(pOutputMemory);
                pOutputMemory = NULL;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynth::renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
        {
            SceHwError nError;

            uint32_t uNumSamplesRemaining = uNumSamplesPerChannel * uNumChannels;
            int16_t *pCurrentBuffer = pBuffer;

            if (uNumChannels != kMaxChannels)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            while(uNumSamplesRemaining != 0)
            {
                int nAvailbleInBuffer = (kNumSamples * kMaxChannels) - nSamplePosition;
                uint32_t uSamplesToCopy;

                if (nAvailbleInBuffer == 0) //we need to generate a new packet
                {
                    nError = process(pOutputMemory);
                    if (nError != SCE_HEATWAVE_API_OK)
                    {
                        return nError;
                    }

                    nSamplePosition = 0;
                    nAvailbleInBuffer = kNumSamples * kMaxChannels;
                }

                uSamplesToCopy = uNumSamplesRemaining;
                if (uSamplesToCopy > nAvailbleInBuffer) uSamplesToCopy = nAvailbleInBuffer;

                memcpy(pCurrentBuffer, &pOutputMemory[nSamplePosition], uSamplesToCopy * sizeof(int16_t));
                
                pCurrentBuffer += uSamplesToCopy;
                uNumSamplesRemaining -= uSamplesToCopy;
                nSamplePosition += uSamplesToCopy;
            }

            m_nSamplesProcessed += uNumSamplesPerChannel;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError NgsSynth::getNgsVoice(int nNumChannels, SceNgsHVoice *pOutVoiceHandle, int32_t *pOutVoiceId)
        {
            int nCnt;
            int returnCode;
            SceNgsHRack CorrectRack = m_RackVoiceMono;
            int nStartIndex = 0;

            if ((nNumChannels < 1) | (nNumChannels > kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (nNumChannels == kMaxChannels)
            {
                nStartIndex += kMaxVoices;
                CorrectRack = m_RackVoiceStereo;
            }

            //Search through voices (from start) and find first available one
            for (nCnt = 0; nCnt < kMaxVoices; nCnt++)
            {
                if (m_pVoiceUsed[nCnt + nStartIndex] == false)
                {
                    SceNgsVoiceInfo VoiceInfo;

                    returnCode = sceNgsRackGetVoiceHandle( CorrectRack, nCnt, pOutVoiceHandle );

                    if (returnCode != SCE_NGS_OK)
                    {
                        return SCE_HEATWAVE_API_ERROR;
                    }

                    sceNgsVoiceGetInfo(*pOutVoiceHandle, &VoiceInfo);

                    if (VoiceInfo.uVoiceState == SCE_NGS_VOICE_STATE_AVAILABLE)
                    {
                        m_pVoiceUsed[nCnt + nStartIndex] = true;
                        *pOutVoiceId = nCnt + nStartIndex;
                        
                        return SCE_HEATWAVE_API_OK;
                    }
                    else
                    {
                        //Clear if not available
                        *pOutVoiceHandle = NULL;
                    }
                }
            }

            return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
        }

        SceHwError NgsSynth::releaseNgsVoice(int32_t pSamplerVoiceRackIndex)
        {
            if ((pSamplerVoiceRackIndex < 0) || (pSamplerVoiceRackIndex >= kMaxVoices * kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_pVoiceUsed[pSamplerVoiceRackIndex] == false)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            sceNgsSystemLock(m_NGSSysHandle);
            sceNgsVoiceKill(m_pVoiceUsed[pSamplerVoiceRackIndex]);
            sceNgsVoiceSetModuleCallback(m_pVoiceUsed[pSamplerVoiceRackIndex], SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_NO_CALLBACK, NULL);
            sceNgsSystemUnlock(m_NGSSysHandle);

            m_pVoiceUsed[pSamplerVoiceRackIndex] = false;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Retrieve information about the synth.
        *
        * @param[out] pInfoOut  Structure to fill out with information about the synth.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError NgsSynth::getInfo(PlatformSynthInfo *pInfoOut) const
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
