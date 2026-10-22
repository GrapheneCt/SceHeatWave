/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_ngs.h
*
* @brief NGS HeatWave synth implementation
*
* @history
*   1.1     Increase kNumSamples from 256 to 512 to prevent AudioOut starvation.
*   1.44    Add functionality to query the number of samples processed.
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_NGS_H_
#define _SCE_HEATWAVE_API_SYNTH_NGS_H_

#include "../heatwave_api_synthinterface.h"
#include <libsysmodule.h>
#include <ngs.h>
#include <ngs/error.h>


namespace sce{
    namespace HeatWave{
        class NgsSynth : public PlatformSynth
        {
        public:
            NgsSynth();
            virtual ~NgsSynth();

            virtual SceHwError doesContextExist(bool *pOutExists);
            SceHwError getNgsVoice(int nNumChannels, SceNgsHVoice *pOutVoiceHandle, int32_t *pnOutVoiceId);
            SceHwError releaseNgsVoice(int32_t nSamplerVoiceRackIndex);
            int32_t getAudioOutPortId();
            bool updateThreadIsRunning();
            SceHwError process(short *pOutput);
            SceNgsHPatch GetPatch(int nIndex);

            static const int32_t kMaxVoices = 64;
            static const int32_t kNumSamples = 512;
            static const int32_t kSampleRate = 48000;
            static const int32_t kMaxChannels = 2;

            SceHwError getInfo(PlatformSynthInfo *pInfoOut) const;

            void incSamplesProcessed(int32_t nNumSamples);

        protected:
            virtual SceHwError createContext();
            virtual SceHwError destroyContext();
            virtual SceHwError renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);

        private:
            SceHwError startNgs();
            SceHwError startAudioOutThread();
            SceHwError startAudioOut();

            SceHwError stopNgs();
            SceHwError stopAudioOutThread();
            SceHwError stopAudioOut();

            int connectRacks( SceNgsHVoice hVoiceSource, SceNgsHVoice hVoiceDest, int nIndex );

            void *m_pSysMem;
            void *m_pRackMemMaster;
            void *m_pRackMemVoiceMono;
            void *m_pRackMemVoiceStereo;

            SceNgsHSynSystem m_NGSSysHandle;
            SceNgsHRack     m_RackMaster;
            SceNgsHRack     m_RackVoiceMono;
            SceNgsHRack     m_RackVoiceStereo;
            
            SceUID  m_ThreadIdAudioUpdate;
            int     m_nAudioOutPortId;
            bool    m_bIsAudioOutAborted;

            SceNgsHVoice MasterVoice;
                        
            bool m_pVoiceUsed[kMaxVoices * 2]; //one for mono and then one for stereo
            int16_t *pOutputMemory;
            SceNgsHPatch patch[kMaxVoices * 2]; //one for mono and then one for stereo
            uint32_t nSamplePosition;
            int32_t m_nSamplesProcessed;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_NGS_H_ */
