/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_multistream.h
*
* @brief Mutlistream HeatWave synth implementation
*
* @history
*   1.2 Remove ::update()
*       Change ::initSynthChannel() to accept InitParams* parameter
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_CHANNEL_MULTISTREAM_H_
#define _SCE_HEATWAVE_API_SYNTH_CHANNEL_MULTISTREAM_H_

#include "heatwave_api_synth_multistream.h"
#include "../heatwave_api_synthinterface.h"

namespace sce{
    namespace HeatWave{
        class MultiStreamSynthChannel : public SynthChannel
        {
        public:
            MultiStreamSynthChannel();
            virtual ~MultiStreamSynthChannel();

            static const float kLowestCutoffFrequency = 20.0f;
            static const int kHighestSampleRate = 192000;
            static const int kNumBuffers = 2;

            virtual SceHwError play();
            virtual SceHwError stop();
            virtual SceHwError pause();

            virtual SceHwError getState(SceHwSfxState *pOutState);

            virtual SceHwError setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels);
            virtual SceHwError setPitch(float32_t fPitch);

            virtual SceHwError setLowpassCutoff(float32_t fLowpassCutoffFactor);

            SceHwError FillBuffer(int nIndex, bool bConsume);
            SceHwError EmptyBuffer();
            
        private:
            virtual SceHwError initSynthChannel(PlatformSynth *pSynth, InitParams *pParam);
            virtual SceHwError releaseSynthChannel();

            SceHwError initPlayerStream( int nSampleRate, uint32_t uNumChannels );
            MultiStreamSynth* m_synth;
            int m_nSampleRate;
            int m_nMultiStreamVoiceID;
            bool m_bPaused;
            int nNumChannels;
            std::queue<SynthBuffer*> m_ngsProcessingBuffers; 
            void *pFilterMemBlock;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_CHANNEL_MULTISTREAM_H_ */
