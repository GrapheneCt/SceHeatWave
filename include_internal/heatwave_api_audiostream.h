/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_audiostream.h
*
* @brief AudioStream is an interface class for sources of audio data (file, oscillator, etc)
*
* @history
*   1.1     heatwave_api_audiostream.h\cpp renamed heatwave_api_audiostream.h\cpp
            sce::HeatWave::DataStream renamed sce::Heatwave::AudioStream
            Clone and take ownership rather than instancing behaviour.
*   1.3     Remove ::seek() and replace with ::reset().
*   1.44    Add ::getDuration() to query the millisecond duration.
*           Add ::seek() to offset to the supplied milli-second time.
*/

#ifndef _SCE_HEATWAVE_AUDIO_STREAM_H_
#define _SCE_HEATWAVE_AUDIO_STREAM_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class AudioStream : public MemoryHandler
        {
        public:
            AudioStream();
            virtual ~AudioStream();

            virtual SceHwError clone(AudioStream **ppCloneStreamOut) const = 0;

            virtual uint32_t    read(int16_t* pBuffer, uint32_t uSizeBytes) =0;
            virtual SceHwError  seek(int32_t nSeekOffset) = 0;

            virtual SceHwError reset() = 0;

            virtual uint32_t    getNumChannels() const =0;
            virtual uint32_t    getSampleRate() const =0;
            virtual int32_t     getDuration() const =0;
            virtual bool        getIsAdpcm() const =0;

        private:
            AudioStream(const AudioStream& rhs); /* Dummies to prevent copy */
            AudioStream& operator=(const AudioStream& rhs);
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_AUDIO_STREAM_H_ */
