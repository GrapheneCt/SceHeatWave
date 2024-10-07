/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "heatwave_api_audiocodec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_wavcodec.h"
#include "heatwave_api_mp3codec.h"
#include "heatwave_api_at9codec.h"
#include "heatwave_api_adpcmcodec.h"
#include "heatwave_api_oggcodec.h"
#include "heatwave_api_opuscodec.h"
#include "heatwave_api.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Detect the format of pDataSource and create a compatible AudioStream
        */
        SceHwError AudioCodec::createAudioCodec(DataSource *pDataSource, AudioCodec **ppStreamOut)
        {
            AudioCodec *stream = NULL;
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pDataSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (ppStreamOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            
            if (_sceHeatWaveIsWaveFormat(pDataSource))  /* WAV */
            {
#if defined SCE_HEATWAVE_CODEC_AT9
#if defined __psp2__
                if (_sceHeatWaveIsAt9Format(pDataSource)) /* ATRAC9 */
                {
                    stream = new At9Codec(pDataSource);
                }
                else
#endif
#endif /* SCE_HEATWAVE_CODEC_AT9 */
                {
                    stream = new WavCodec(pDataSource);
                }
            }
#if defined SCE_HEATWAVE_CODEC_MP3
            else if (SCE_HEATWAVE_API_SUCCEEDED(Mp3Codec::isMp3Format(pDataSource))) /* MP3 */
            {
                stream = new Mp3Codec(pDataSource);
            }
#endif /* SCE_HEATWAVE_CODEC_MP3 */
#if defined SCE_HEATWAVE_CODEC_OGG
            else if (SCE_HEATWAVE_API_SUCCEEDED(OggCodec::isOggFormat(pDataSource))) /* OGG Vorbis */
            {
                stream = new OggCodec(pDataSource);
            }
#endif /* SCE_HEATWAVE_CODEC_OGG */
#if defined SCE_HEATWAVE_CODEC_OPUS
            else if (SCE_HEATWAVE_API_SUCCEEDED(OpusCodec::isOpusFormat(pDataSource))) /* OPUS */
            {
                stream = new OpusCodec(pDataSource);
            }
#endif /* SCE_HEATWAVE_CODEC_OPUS */
#if defined SCE_HEATWAVE_CODEC_HEVAG
#if defined __psp2__
            else if (_sceHeatWaveIsAdpcmFormat(pDataSource)) /* ADPCM */
            {
                stream = new AdpcmCodec(pDataSource);
            }
#endif
#endif /* SCE_HEATWAVE_CODEC_HEVAG */
            else
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            if (stream == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            res = stream->init();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                stream->release();
                delete stream;

                return res;
            }

            *ppStreamOut = stream;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Check the format of pDataSource
        */
        SceHwError AudioCodec::validAudioCodec(DataSource *pDataSource)
        {
            SceHwError res = SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;

            if (pDataSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (_sceHeatWaveIsWaveFormat(pDataSource))  /* WAV */
            {
                res = SCE_HEATWAVE_API_OK;
            }
#if defined SCE_HEATWAVE_CODEC_MP3
            else if (SCE_HEATWAVE_API_SUCCEEDED(Mp3Codec::isMp3Format(pDataSource))) /* MP3 */
            {
                res = SCE_HEATWAVE_API_OK;
            }
#endif /* SCE_HEATWAVE_CODEC_MP3 */
#if defined SCE_HEATWAVE_CODEC_OGG
            else if (SCE_HEATWAVE_API_SUCCEEDED(OggCodec::isOggFormat(pDataSource))) /* OGG Vorbis */
            {
                res = SCE_HEATWAVE_API_OK;
            }
#endif /* SCE_HEATWAVE_CODEC_OGG */
#if defined SCE_HEATWAVE_CODEC_OPUS
            else if (SCE_HEATWAVE_API_SUCCEEDED(OpusCodec::isOpusFormat(pDataSource))) /* OPUS */
            {
                res = SCE_HEATWAVE_API_OK;
            }
#endif /* SCE_HEATWAVE_CODEC_OPUS */
#if defined SCE_HEATWAVE_CODEC_HEVAG
#if defined __psp2__
            else if (_sceHeatWaveIsAdpcmFormat(pDataSource)) /* ADPCM */
            {
                res = SCE_HEATWAVE_API_OK;
            }
#endif
#endif /* SCE_HEATWAVE_CODEC_HEVAG */

            return res;
        }

        AudioCodec::AudioCodec(DataSource *pSource)
            : MemoryHandler(),
            m_dataSource(pSource),
            m_numberOfChannels(0),
            m_sampleRate(0),
            m_nDuration(knDurationUnknown),
            m_bIsAdpcm(false)
        {
        }


        AudioCodec::~AudioCodec()
        {}

        void AudioCodec::release()
        {
            m_dataSource = NULL;
            m_numberOfChannels = 0;
            m_sampleRate = 0;
            m_bIsAdpcm = false;
        }

        /**
        * @brief Seek to specified time position.
        *
        * @param[in] nSeekOffset The desired millisecond seek position.
        *
        * Seek to the supplied milli-second time.
        */
        SceHwError AudioCodec::seek(int32_t nSeekOffset)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = performSeek(nSeekOffset);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read data as 16bit PCM
        */
        SceHwError AudioCodec::read(int16_t *pBuffer, uint32_t uSizebytes, uint32_t *pNumBytesReadOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pBuffer == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pNumBytesReadOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = performRead(pBuffer, uSizebytes, pNumBytesReadOut);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Query the number of channels
        */
        uint32_t AudioCodec::getNumberOfChannels() const
        {
            return m_numberOfChannels;
        }

        /**
        * @brief The sample frequency of the audio data
        */
        uint32_t AudioCodec::getSampleRate() const
        {
            return m_sampleRate;
        }

        /**
        * @brief The millisecond duration of the audio data.
        *
        * @return The duration in milliseconds.
        */
        int32_t AudioCodec::getDuration() const
        {
            return m_nDuration;
        }

        bool AudioCodec::getIsAdpcm() const
        {
            return m_bIsAdpcm;
        }

    } /* HeatWave */
} /* sce */
