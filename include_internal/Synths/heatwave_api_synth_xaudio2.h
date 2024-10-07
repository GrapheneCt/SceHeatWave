/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_xaudio2.h
*
* @brief XAudio2 HeatWave synth implementation
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_XAUDIO2_H_
#define _SCE_HEATWAVE_API_SYNTH_XAUDIO2_H_

#include "../heatwave_api_synthinterface.h"
#include <xaudio2.h>

namespace sce{
    namespace HeatWave{

        class XAudio2Synth : public PlatformSynth
        {
        public:
            XAudio2Synth();
            virtual ~XAudio2Synth();

            virtual SceHwError doesContextExist(bool *pOutExists);

            SceHwError getXAudio2Voice(const WAVEFORMATEX *pXAudio2Format, IXAudio2SourceVoice **pXAudio2VoiceOut);
            SceHwError getXAudio2MasterVoice(IXAudio2MasteringVoice** pXAudio2MasterVoiceOut);

        protected:
            virtual SceHwError createContext();
            virtual SceHwError destroyContext();
            virtual SceHwError renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);

        private:
            SceHwError createXAudio2Engine();
            SceHwError createXAudio2MasterVoice();

            IXAudio2* m_xaudio2Engine;
            IXAudio2MasteringVoice* m_xaudio2MasterVoice;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_XAUDIO2_H_ */
