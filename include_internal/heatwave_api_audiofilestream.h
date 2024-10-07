/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_audiofilestream.h
*
* @brief An AudioStream that uses an audio file as it's source of data
*
* FileSource ----> DataSource ----> AudioFileStream ----> AudioStream
*                      ^
* MemorySource --------|
*
* @history
*   1.1     heatwave_api_audiofileinternal.h\cpp renamed heatwave_api_audiofilestream.h\cpp
*           sce::Heatwave::AudioFileInteral renamed sce::HeatWave::AudioFileStream
*           Add ::clone() to create a new independant instance of the AudioFileStream
*           ::init() accepts a DataSource, which it creates a clone of for use
*           Remove ::initDataSource()
*           Remove ::tell()
*           Use AudioCodec to get pcm audio data from DataSource
*   1.3     Remove ::seek().
*           Add ::reset().
*   1.44    Add ::getDuration().
*           Add ::seek().
*/

#ifndef _SCE_HEATWAVE_API_AUDIO_FILE_STREAM_H_
#define _SCE_HEATWAVE_API_AUDIO_FILE_STREAM_H_

#include "heatwave_api_types.h"
#include "heatwave_api_audiostream.h"
#include "heatwave_api_datasource.h"

namespace sce{
    namespace HeatWave{

        class DataSource;
        class AudioCodec;

        class AudioFileStream : public AudioStream
        {
        public:
            AudioFileStream();
            ~AudioFileStream();

            SceHwError clone(AudioStream **ppCloneStreamOut) const;

            SceHwError init(const DataSource *pDataSource, bool bBasicInit);
            SceHwError release();
            bool isOpen() const;

            SceHwError  reset();
            uint32_t    read(int16_t *pBuffer, uint32_t uNumBytes);
            SceHwError  seek(int32_t nSeekOffset);

            uint32_t    getNumChannels() const;
            uint32_t    getSampleRate() const;
            int32_t     getDuration() const;
            bool        getIsAdpcm() const;

        private:
            AudioCodec *m_codec;
            DataSource* m_dataSource; ///< Source of data (File\Memory)

            AudioFileStream(const AudioFileStream& rhs); /* Dummies to prevent copy */
            AudioFileStream& operator=(const AudioFileStream& rhs);
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_AUDIO_FILE_STREAM_H_ */
