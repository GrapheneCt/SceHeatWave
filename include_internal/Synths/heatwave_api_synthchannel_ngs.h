/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_ngs.h
*
* @brief NGS HeatWave synth implementation
*
* @history
*   1.2     Remove ::update()
*           Change ::initSynthChannel() to accept InitParams* parameter.
*   1.44    Fix ::FillBuffer() memory access, check m_buffersToProcess before access.
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS_H_
#define _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS_H_

#include "../heatwave_api_synthinterface.h"
#include <libsysmodule.h>
#include <ngs.h>
#include <ngs/error.h>


namespace sce{
    namespace HeatWave{

        class NgsSynthChannel : public SynthChannel
        {
        public:
            NgsSynthChannel();
            virtual ~NgsSynthChannel();

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

            int initPlayerStream( SceNgsHVoice hVoice, int nSampleRate, int nNumChannels, bool isAdpcm );

            NgsSynth* m_synth;
            void *m_ngsVoiceHandle;
            SceNgsHVoice VoiceHandle;
            int nVoiceIndex;
            bool bIsPaused;
            std::queue<SynthBuffer*> m_ngsProcessingBuffers; 
        };
    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_CHANNEL_NGS_H_ */
