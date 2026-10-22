/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_channel_ngs2.h
*
* @brief Ngs2 HeatWave synth channel implementation
*
* @history
*   1.2 Change : calls to NGS2 sampler specific functions wrapped virtual member functions to allow overrides.
*   1.3 Change ::play to not issue a SCE_NGS2_VOICE_EVENT_PLAY event if the voice is already processing data.
*   1.4 Change ::setVolumeMatrix to remove allocation & free of volume array.
*       Change ::initSynthChannel to allow alternative output destinations.
*       Remove m_ngs2VoiceIndex, the voice index can be obtained through NGS2.
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS2_H_
#define _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS2_H_

#include "../heatwave_api_synthinterface.h"
#include <ngs2.h>

namespace sce{
    namespace HeatWave{

        class Ngs2Synth;

        class Ngs2SynthChannel : public SynthChannel
        {
        public:
            Ngs2SynthChannel();
            virtual ~Ngs2SynthChannel();

            virtual SceHwError update();

            virtual SceHwError play();
            virtual SceHwError stop();
            virtual SceHwError pause();

            virtual SceHwError getState(SceHwSfxState *pOutState);

            virtual SceHwError setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels);
            virtual SceHwError setPitch(float32_t fPitch);

            virtual SceHwError setLowpassCutoff(float32_t fLowpassCutoffFactor);

        protected:
            SceHwError initSynthChannel(PlatformSynth *pSynth, InitParams *pParam);
            SceHwError releaseSynthChannel();

            virtual SceHwError setupNgs2Voice(SceNgs2Handle hVoiceHandle, SceNgs2WaveformFormat *pWaveFormat, uint32_t uFlags = 0);
            virtual SceHwError setNgs2VoiceFilterByFcq(SceNgs2Handle hVoiceHandle, uint32_t uIndex, uint32_t uLocation, uint32_t uType, uint32_t uChannelMask, float fC, float fQ, float fLevel);
            virtual SceHwError addNgs2VoiceWaveformBlocks(SceNgs2Handle hVoiceHandle, const void *pData, const SceNgs2WaveformBlock *pBlock, uint32_t uNumBlocks, uint32_t uFlags);
            virtual SceHwError getNgs2VoiceDecodedDataSize(SceNgs2Handle hVoiceHandle, uint64_t *pDecodedDataSizeOut);

            typedef struct Ngs2ProcessBuffer : public MemoryHandler
            {
                uint32_t uByteOffset;
                SynthBuffer *pBuffer;
            } Ngs2ProcessBuffer;

            SceHwError consumeBuffers();
            SceHwError processBuffers();
            SceHwError updateState();

            SceHwSfxState m_state;
            SceNgs2Handle m_ngs2VoiceHandle;
            SceNgs2WaveformFormat m_ngs2Format;
            uint32_t m_bytesQueued;
            std::queue<Ngs2ProcessBuffer*> m_ngs2ProcessingBuffers; ///< Buffers of data ngs2 is currently processing
            Ngs2Synth* m_synth;
            float32_t *m_fLevels;
            int32_t m_nDestination;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS2_H_*/
