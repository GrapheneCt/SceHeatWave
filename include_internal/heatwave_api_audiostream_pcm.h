/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_audiostream_pcm.h
*
* @brief Manage calls to generate audio data at runtime via user supplied callbacks.
*
* @history
*   1.3     File added.
*   1.44    Add ::getDuration().
*/

#ifndef _SCE_HEATWAVE_API_AUDIO_STREAM_PCM_H_
#define _SCE_HEATWAVE_API_AUDIO_STREAM_PCM_H_

#include "heatwave_api_audiostream.h"

namespace sce{
    namespace HeatWave{

        class AudioStreamPCM : public AudioStream
        {
        public:
            AudioStreamPCM();
            ~AudioStreamPCM();

            SceHwError clone(AudioStream **ppCloneStreamOut) const;

            SceHwError init(const SceHwSourceRuntimeStreamParam* pParam, SceHwSourceRuntimeStreamCallbackFunction pFunc, void *pUserData);
            SceHwError reset();

            uint32_t read(int16_t* pnBuffer, uint32_t uSizeBytes);
            SceHwError seek(int32_t nSeekOffset);

            uint32_t    getNumChannels() const;
            uint32_t    getSampleRate() const;
            int32_t     getDuration() const;
            bool        getIsAdpcm() const;

        private:
            SceHwSourceRuntimeStreamCallbackFunction m_pRenderFunction;
            void *m_pUserData;
            SceHwSourceRuntimeStreamCallbackInfo m_info;
            SceHwSourceRuntimeStreamParam m_param;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_AUDIO_STREAM_PCM_H_ */
