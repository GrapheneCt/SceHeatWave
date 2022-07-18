/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_voiceinternal
*
* @description
*   Active instance of a SourceInternal used for playback
*   Multiple VoiceInternal can point to the same HeatWave_SourceInteral
*
*   VoiceInternal ---> SourceInternal ---> AudioStream
*   VoiceInternal -----^
*
*   VoiceInternal ---> AudioStream
*
* @history
*   1.1     Process AudioStream data in byte blocks rather than frame blocks
*   1.2     Change ::initialise() to use SceHwSfxPlayParam and pass on parameters for creation of the SynthChannel
*           Rename ::updateSurroundSettings() to ::updateParameters()
*           Change volume and pitch to account for the values of both the SceHwSource and SceHwSfx
*   1.44    Add ::seek() to set the current playback position.
*           Add ::reset() to return to initial state.
*           Track SynthBuffers not in use by the SynthChannel in m_availBuffers.
*/

#ifndef _SCE_HEATWAVE_API_VOICE_INTERNAL_H_
#define _SCE_HEATWAVE_API_VOICE_INTERNAL_H_

#include "heatwave_api_types.h"
#include "heatwave_api.h"
#include "heatwave_api_audiostream.h"
#include "heatwave_api_memory.h"
#include "heatwave_api_synthinterface.h"

namespace sce{
    namespace HeatWave{

        class SourceInternal;
        class SfxInternal;

        class VoiceInternal : public MemoryHandler
        {
        public:
            VoiceInternal(SfxInternal *pParent);
            ~VoiceInternal();

            bool initialise(SourceInternal *pSource, SceHwSfxPlayParam *pParam);
            void release();

            bool play();
            bool stop();

            bool pause();
            bool resume();

            SceHwError update();
            SceHwError updateParameters();

            SceHwError setVolume(float32_t fVol);
            SceHwError setPitch(float32_t fPitchMul);
            SceHwError setPosition(SceHwVector vPos);
            SceHwError setVelocity(SceHwVector vVel);
            SceHwError setForward(SceHwVector vDir);
            SceHwError setMinMaxDistance(float32_t fMin, float32_t fMax);
            SceHwError setDistanceFactor(float32_t fFactor);
            SceHwError setCone(float32_t fInsideAng, float32_t fOutsideAng, float32_t fOutsideGain, float32_t fOutsideFreq);
            SceHwError setPan(float32_t fPan);
            SceHwError getType(SceHwSourceType *pTypeOut);

            SceHwError getState(SceHwSfxState *pStateOut);

            SceHwError seek(int32_t nSeekTimeMs);
            SceHwError reset();

            static const uint32_t kuNumBuffers = 3;           ///< Number of buffers for streaming (1 reading, 1 ready, 1 playing)
            static const uint32_t kuBufferSizeBytes = 8 * 1024;    ///< Size of each stream buffer

        private:
            SceHwError apply3DSettings();
            SceHwError applySourceSettings();
            bool queueBuffer(SynthChannel::SynthBuffer *pBuffer);

            SynthChannel* m_channel;
            SynthChannel::SynthBuffer m_buffer[kuNumBuffers];
            std::vector<SynthChannel::SynthBuffer*> m_availBuffers;

            int32_t m_loopsRemaining;

            SourceInternal* m_sourceHandle;
            SfxInternal* m_parentSfx;

            AudioStream* m_audioStream;

            static uint32_t s_globalVoiceCount;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_VOICE_INTERNAL_H_ */
