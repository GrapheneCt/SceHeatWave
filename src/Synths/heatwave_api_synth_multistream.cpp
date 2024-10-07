/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "Synths\heatwave_api_synth_multistream.h"
#include "heatwave_api_systeminternal.h"
#include <stdio.h>

#include <sysutil/sysutil_sysparam.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>                          
#include <sys/ppu_thread.h>
#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/process.h>
#include <sys/spu_initialize.h>


#include <fcntl.h>
#include <cell/fs/cell_fs_errno.h>
#include <cell/fs/cell_fs_file_api.h>

#include <cell/mstream.h>

#include <assert.h>

#include <sys/paths.h>
#include <cell/audio.h>
#include <cell/gcm.h>

#include <sys/system_types.h>

#include <cell/spurs/control.h>
#include <cell/spurs/task.h>
#include <cell/spurs/event_flag.h>
#include <cell/sysmodule.h>

#define MAX_SHORT (32767)
#define MAX_SHORT_FLOAT (32767.0f)

//Filter DSP entry point
extern char _binary_mstream_dsp_filter_pic_start[];

namespace sce{
    namespace HeatWave{
        long StartMultiStreamUpdateThread(void _thread (uint64_t param));
        static void _Multi_Stream_Update_Thread(uint64_t param);

        /**
        * @brief pass ngs allocs into HeatWave (user overrides)
        */
        void *_multistreamMemAlign(uint32_t nSize, uint32_t nAlign)
        {
            return MemoryHandler::memalignCb(nAlign, nSize); 
        }

        /**
        * @brief pass ngs allocs into HeatWave (user overrides)
        */
        void *_multistreamMalloc(uint32_t nSize)
        {
            return MemoryHandler::mallocCb(nSize);
        }

        /**
        * @brief pass ngs free into HeatWave (user overrides)
        */
        void _multistreamFree(void *pPtr)
        {
            MemoryHandler::freeCb(pPtr);
        }

        PlatformSynth* PlatformSynth::createSynth()
        {
            return new MultiStreamSynth();
        }

        MultiStreamSynth::MultiStreamSynth()
            : PlatformSynth(),
            bSynthStarted(false),
            s_MultiStreamPuThread(NULL),
            m_bIsAudioOutAborted(false),
            nSamplePosition(kGrainSize), //Set to the end of the buffer, as no data is available at start.
            m_pMultiStreamMemory(NULL),
            nPortNum(0),
            m_nSamplesProcessed(0)
        {
            m_numOutputChannels = MultiStreamSynth::kMaxChannels;
        }

        MultiStreamSynth::~MultiStreamSynth()
        {
            destroyContext(); //clean up
        }

        SceHwError MultiStreamSynth::getMultiStreamVoice(int *pOutVoiceHandle)
        {
            int nChannelID;

            if (pOutVoiceHandle == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            nChannelID = cellMSStreamOpen();

            if (nChannelID < 0)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            *pOutVoiceHandle = nChannelID;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::releaseMultiStreamVoice(int nHandle)
        {
            cellMSStreamClose(nHandle); //No error return from close

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::startMultiStream()
        {
            CellMSSystemConfig cfg;
            int nMemoryNeeded;
            int nReturn;
            CellMSDSP DSPInfo;

            float fBusVols[64] = 
                {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            
            cfg.channelCount = kMaxVoices;
            cfg.subCount = kMaxSubs;
            cfg.dspPageCount = 1; //filter page only

            if (!m_loopback)
            {
                cfg.flags = CELL_MS_TD_ONLY_256;    
            }
            else
            {
                cfg.flags = CELL_MS_EXTERNAL_BUFFERS | CELL_MS_TD_ONLY_256; 
            }
            
            nMemoryNeeded = cellMSSystemGetNeededMemorySize(&cfg);
            m_pMultiStreamMemory = _multistreamMemAlign(nMemoryNeeded, 128); 
            
            nReturn = cellMSSystemInitSPUThread(m_pMultiStreamMemory, &cfg, kSPUThreadPriority);
            if (nReturn == -1)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            nDSPHandle = cellMSDSPLoadDSPFromMemory((void*)_binary_mstream_dsp_filter_pic_start, &DSPInfo, CELL_MS_ALLOC_DSPHANDLE,CELL_MS_ALLOC_PAGE);
            if (nDSPHandle == -1)
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            // Setup the final output channel volumes
            cellMSCoreSetVolume64(CELL_MS_MASTER_BUS, CELL_MS_DRY, fBusVols);

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::stopMultiStream()
        {
            if (bSynthStarted == true)
            {
                cellMSSystemClose();

                _multistreamFree(m_pMultiStreamMemory);
            }

            bSynthStarted = false;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::createContext()
        {
            SceHwError nError;

            nError = startMultiStream();
            if (nError != SCE_HEATWAVE_API_OK)
            {
                return nError;
            }
            
            m_bIsAudioOutAborted = false;

            if (!m_loopback)
            {
                nError = startAudioOutThread();

                if (nError != SCE_HEATWAVE_API_OK)
                {
                    m_bIsAudioOutAborted = true;
                    return nError;
                }
            }

            bSynthStarted = true;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::destroyContext()
        {
            SceHwError nError;

            m_bIsAudioOutAborted = true;
            
            if (!m_loopback)
            {
                nError = stopAudioOutThread();

                if (nError != SCE_HEATWAVE_API_OK)
                {
                    return nError;
                }
            }

            nError = stopMultiStream();     
            if (nError != SCE_HEATWAVE_API_OK)
            {
                return nError;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::doesContextExist(bool *pOutExists)
        {
            if (pOutExists == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pOutExists = bSynthStarted;
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
        {
            SceHwError nError;
            uint32_t uSampleCount;
            uint32_t nChannelCount;
            int16_t *pInput;
            uint32_t uNumSamplesRemaining = uNumSamplesPerChannel;
            int16_t *pCurrentBuffer = pBuffer;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if ((uNumChannels < 1) | (uNumChannels >= kMaxChannels))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (!m_loopback)
            {
                return SCE_HEATWAVE_API_OK;
            }
            
            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            while(uNumSamplesRemaining != 0)
            {
                uint32_t uSamplesToCopy;
                int nAvailbleInBuffer = kGrainSize - nSamplePosition;
                if (nAvailbleInBuffer < 0)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }

                if (nAvailbleInBuffer == 0) //we need to generate a new packet
                {
                    nError = process(OutputMemory);
                    if (nError != SCE_HEATWAVE_API_OK)
                    {
                        return nError;
                    }

                    nSamplePosition = 0;
                    nAvailbleInBuffer = kGrainSize;
                }

                uSamplesToCopy = uNumSamplesRemaining;
                if (uSamplesToCopy > (uint32_t)nAvailbleInBuffer) uSamplesToCopy = nAvailbleInBuffer;

                pInput = &OutputMemory[nSamplePosition * kMaxChannels];

                for (uSampleCount = 0; uSampleCount < uSamplesToCopy; uSampleCount++)
                {
                    if (uNumChannels == 2) //perform downmix
                    {
                         float fValue = (float)pInput[0] + ((float)pInput[2] * 0.707f) + ((float)pInput[4] * 0.707f)+ ((float)pInput[6] * 0.707f);

                         if (fValue > MAX_SHORT_FLOAT) 
                            pCurrentBuffer[0] = MAX_SHORT;
                         else if (fValue < -MAX_SHORT_FLOAT)
                             pCurrentBuffer[0] = -MAX_SHORT;
                         else
                             pCurrentBuffer[0] = (int16_t)fValue;
                             
                        fValue = (float)pInput[1] + ((float)pInput[2] * 0.707f) + ((float)pInput[5] * 0.707f)+ ((float)pInput[7] * 0.707f);

                        if (fValue > MAX_SHORT_FLOAT) 
                            pCurrentBuffer[1] = MAX_SHORT;
                        else if (fValue < -MAX_SHORT_FLOAT)
                            pCurrentBuffer[1] = -MAX_SHORT;
                        else
                            pCurrentBuffer[1] = (int16_t)fValue;
                    }
                    else
                    {
                        for (nChannelCount = 0; nChannelCount < uNumChannels; nChannelCount++)
                        {
                            pCurrentBuffer[nChannelCount] = pInput[nChannelCount];
                        }
                    }

                    pInput += kMaxChannels;
                    pCurrentBuffer += uNumChannels;
                }

                uNumSamplesRemaining -= uSamplesToCopy;
                nSamplePosition += uSamplesToCopy;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::process(int16_t *pBuffer)
        {
            uint32_t uCnt;

            float *pBufferMultiStream = (float *)cellMSSystemSignalSPU();

            if (pBufferMultiStream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            if (pBuffer != NULL)
            {
                int nValue;

                for (uCnt = 0; uCnt < kGrainSize * kMaxChannels; uCnt++)
                {
                    nValue = (int)(pBufferMultiStream[uCnt] * MAX_SHORT_FLOAT);

                    if (nValue > MAX_SHORT) 
                    {
                        nValue = MAX_SHORT;
                    }
                    else if (nValue < -MAX_SHORT) 
                    {
                        nValue = -MAX_SHORT;
                    }

                    pBuffer[uCnt] = nValue;
                }
            }

            if (SystemInternal::getInstance() != NULL)
            {
                SCE_HEATWAVE_LOCK();
                cellMSSystemGenerateCallbacks();
                SCE_HEATWAVE_UNLOCK();
            }

            m_nSamplesProcessed += kGrainSize;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::startAudioOutThread()
        {
            SceHwError nReturn;
            int         nRet;

            nReturn = audioInitCell(); //start our low level audio port

            if (nReturn != SCE_HEATWAVE_API_OK)
            {
                return nReturn;
            }

            // create the MultiStream / libaudio update thread
            nRet = sys_ppu_thread_create(&s_MultiStreamPuThread, _Multi_Stream_Update_Thread, (uint64_t)this, 0, 0x4000, SYS_PPU_THREAD_CREATE_JOINABLE, "MultiStream PU Thread");
            if(nRet)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError MultiStreamSynth::stopAudioOutThread()
        {
            int nReturn;
            uint64_t nRetFromThread;

            nReturn = sys_ppu_thread_join(s_MultiStreamPuThread, &nRetFromThread);
            if (nReturn != 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nReturn = audioShutdownCell();
            if (nReturn != SCE_HEATWAVE_API_OK)
            {
                return nReturn;
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool MultiStreamSynth::updateThreadIsRunning()
        {
            return !m_bIsAudioOutAborted;
        }

        static void _Multi_Stream_Update_Thread(uint64_t param)
        {
            MultiStreamSynth *pSynth = (MultiStreamSynth *)((int)param);

            while(pSynth->updateThreadIsRunning())
            {
                pSynth->process(NULL);
            }
            
            sys_ppu_thread_exit(0);
        }

        SceHwError MultiStreamSynth::audioShutdownCell()
        {
            int nRet;
            nRet = cellAudioPortStop(nPortNum);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = cellAudioPortClose(nPortNum);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;

        }

        SceHwError MultiStreamSynth::audioInitCell()
        {
            CellAudioPortParam   AudioParam;
            CellAudioPortConfig  PortConfig;
            int nRet;

            // audio port open.
            memset(&AudioParam, 0, sizeof(CellAudioPortParam));
            AudioParam.nChannel = CELL_AUDIO_PORT_8CH;
            AudioParam.nBlock   = CELL_AUDIO_BLOCK_8;

            nRet = cellAudioPortOpen(&AudioParam, &nPortNum);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
            }

            // get port config.
            nRet = cellAudioGetPortConfig(nPortNum, &PortConfig);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = cellMSSystemConfigureLibAudio(&AudioParam, &PortConfig);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = cellAudioPortStart(nPortNum);
            if (nRet != CELL_OK)
            {
                cellAudioQuit();
                return SCE_HEATWAVE_API_ERROR;
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
        SceHwError MultiStreamSynth::getInfo(PlatformSynthInfo *pInfoOut) const
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
