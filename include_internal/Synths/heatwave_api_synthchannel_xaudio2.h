/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synthchannel_xaudio2.h
*
* @brief XAudio2 HeatWave synth channel implementation
*
* @history
*   1.2 Change ::initSynthChannel() to accept InitParams* parameter.
*       Change ::update to call the parent class ::update.
*       Remove eChannelType parameter from ::getWaveFormatX.
*   1.4 Fix ::setVolumeMatrix access of m_synth before NULL check.
*       Fix unused variable kNumLevels in ::setVolumeMatrix.
*       Change ::setVolumeMatrix to remove allocation & free of volume array.
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_CHANNEL_XAUDIO2_H_
#define _SCE_HEATWAVE_API_SYNTH_CHANNEL_XAUDIO2_H_

#define XAUDIO2_HELPER_FUNCTIONS

#include "../heatwave_api_synthchannelinterface.h"
#include "heatwave_api_synth_xaudio2.h"
#include <XAudio2.h>

namespace sce{
    namespace HeatWave{

        class XAudio2SynthChannel : public SynthChannel
        {
        public:
            XAudio2SynthChannel();
            virtual ~XAudio2SynthChannel();

            virtual SceHwError update();

            virtual SceHwError play();
            virtual SceHwError stop();
            virtual SceHwError pause();

            virtual SceHwError getState(SceHwSfxState *pOutState);

            virtual SceHwError setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels);
            virtual SceHwError setPitch(float32_t fPitch);

            virtual SceHwError setLowpassCutoff(float32_t fLowpassCutoffFactor);

        private:
            virtual SceHwError initSynthChannel(PlatformSynth *pSynth, InitParams *pParam);
            virtual SceHwError releaseSynthChannel();

            SceHwError getWaveFormatX(uint32_t uNumChannels, uint32_t uSampleRate, WAVEFORMATEX* pXaudio2FormatOut);

            SceHwError consumeBuffers();
            SceHwError processBuffers();

            IXAudio2SourceVoice* m_xaudio2Voice;
            SceHwSfxState m_state;
            uint32_t m_numChannels;
            std::queue<SynthBuffer*> m_xaudio2ProcessingBuffers;
            IXAudio2MasteringVoice* m_xaudio2MasterVoice;
            XAudio2Synth* m_synth;
            float32_t* m_fLevels;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_CHANNEL_XAUDIO2_H_ */
