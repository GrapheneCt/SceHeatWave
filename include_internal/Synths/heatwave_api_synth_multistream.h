/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_multistream.h
*
* @brief Mutlistream HeatWave synth implementation
*
* @history
*   1.44    Add functionality to query the number of samples processed.
*/
#ifndef _SCE_HEATWAVE_API_SYNTH_MULTISTREAM_H_
#define _SCE_HEATWAVE_API_SYNTH_MULTISTREAM_H_

#include "../heatwave_api_synthinterface.h"
#include <cell/mstream.h>


namespace sce{
    namespace HeatWave{

        class MultiStreamSynth : public PlatformSynth
        {
        public:
            MultiStreamSynth();
            virtual ~MultiStreamSynth();

            virtual SceHwError doesContextExist(bool *pOutExists);

            static const int32_t kMaxVoices = 256;
            static const int32_t kMaxSubs = 1;
            static const int32_t kSPUThreadPriority = 100;
            static const uint32_t kMaxChannels = 8;
            static const int32_t kGrainSize = 256;
            static const int32_t kSampleRate = 48000;

            SceHwError getMultiStreamVoice(int *pOutVoiceHandle);
            SceHwError releaseMultiStreamVoice(int nHandle);
            bool updateThreadIsRunning();
            SceHwError process(int16_t *pBuffer);

            SceHwError getInfo(PlatformSynthInfo *pInfoOut) const;

            int nDSPHandle;

        protected:
            virtual SceHwError createContext();
            virtual SceHwError destroyContext();
            virtual SceHwError renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);

        private:
            SceHwError startAudioOutThread();
            SceHwError startMultiStream();

            SceHwError stopAudioOutThread();
            SceHwError stopMultiStream();

            SceHwError audioInitCell();
            SceHwError audioShutdownCell();


            bool bSynthStarted;
            sys_ppu_thread_t s_MultiStreamPuThread;
            bool m_bIsAudioOutAborted;
            int nSamplePosition;

            int16_t OutputMemory[kMaxChannels * kGrainSize];
            void *m_pMultiStreamMemory;
            uint32_t nPortNum;

            int32_t m_nSamplesProcessed;

        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_MULTISTREAM_H_ */
