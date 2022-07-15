/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_audiocodec.h
*
* @brief Top level interface to abstract different audio data formats
*
* @history
*   1.1     File added.
*   1.44    Add ::getDuration() to query millisecond duration, and m_nDuration 
*               to cache duration when initialising the AudioCodec.
*           Change ::seek() and ::performSeek() from a byte to milli-second offset.
*/

#ifndef _SCE_HEATWAVE_API_AUDIO_CODEC_H_
#define _SCE_HEATWAVE_API_AUDIO_CODEC_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class DataSource;

        class AudioCodec : public MemoryHandler
        {
        public:
            static SceHwError createAudioCodec(DataSource *pDataSource, AudioCodec **ppStreamOut); ///< Factory
            virtual ~AudioCodec();
            void release();

            SceHwError seek(int32_t nSeekOffset);
            SceHwError read(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut);

            uint32_t    getNumberOfChannels() const;
            uint32_t    getSampleRate() const;
            int32_t     getDuration() const;
            bool        getIsAdpcm() const;

        protected:
            virtual SceHwError init() =0;

            virtual SceHwError performSeek(int32_t nSeekOffset) = 0;
            virtual SceHwError performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut) =0;

            AudioCodec(DataSource *pSource); ///< Creation should be done using the ::createAudioCodec() factory method

            DataSource* m_dataSource;

            uint32_t m_numberOfChannels;
            uint32_t m_sampleRate;
            int32_t m_nDuration;
            bool m_bIsAdpcm;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_AUDIO_CODEC_H_ */
