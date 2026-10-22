/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_OGG

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "heatwave_api_oggcodec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"
#include "heatwave_api.h"

/**
* @brief Multiplier to covert from seconds to milliseconds.
*
* 1sec == 1000mS
*/
#define SECS_TO_MS (1000.0f)

extern "C"
{

    #undef _ogg_malloc
    #undef _ogg_calloc
    #undef _ogg_realloc
    #undef _ogg_free

    void *_ogg_malloc(unsigned int size)
    {
        return sce::HeatWave::MemoryHandler::mallocCb(size);
    }

    void *_ogg_calloc(unsigned int num, unsigned int size)
    {
        return sce::HeatWave::MemoryHandler::callocCb(num, size);
    }

    void *_ogg_realloc(void *ptr, unsigned int size)
    {
        return sce::HeatWave::MemoryHandler::reallocCb(ptr, size);
    }

    void _ogg_free(void *ptr)
    {
        sce::HeatWave::MemoryHandler::freeCb(ptr);
    }

    void *_ogg_memcpy(void *dst, void *src, size_t size)
    {
        return sce::HeatWave::MemoryHandler::memcpyCb(dst, src, size);
    }

    void _ogg_memset(void *mem, int32_t val, size_t size)
    {
        sce::HeatWave::MemoryHandler::memsetCb(mem, val, size);
    }

    void *_ogg_memmove(void *dst, void *src, size_t size)
    {
        return sce::HeatWave::MemoryHandler::memmoveCb(dst, src, size);
    }
}

namespace sce{
    namespace HeatWave{

        uint32_t OggCodec::OggRead(void *ptr, uint32_t size, uint32_t nmemb, void *datasource)
        {
            DataSource *pStream = (DataSource *)datasource;

            return pStream->read(ptr, size, nmemb);
        }

        int32_t OggCodec::OggSeek(void *datasource, ogg_int64_t offset, int32_t whence)
        {
            DataSource *pStream = (DataSource *)datasource;

            return pStream->seek((int32_t)offset, whence);
        }

        long OggCodec::OggTell(void *datasource)
        {
            DataSource *pStream = (DataSource *)datasource;

            return pStream->tell();
        }

        int32_t OggCodec::OggClose(void *datasource)
        {
            return 0;
        }

        /**
        * @brief Verify pStream is OGG Vorbis format
        */
        SceHwError OggCodec::isOggFormat(DataSource *pStream)
        {
            int32_t ret = 0;
            OggVorbis_File ogg;
            ov_callbacks ioCb;

            ioCb.read_func = OggRead;
            ioCb.seek_func = OggSeek;
            ioCb.tell_func = OggTell;
            ioCb.close_func = OggClose;

            pStream->seek(0, SEEK_SET);

            ret = ov_test_callbacks(pStream, &ogg, NULL, 0, ioCb);
            if (ret != 0)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            ov_clear(&ogg);

            pStream->seek(0, SEEK_SET);

            return SCE_HEATWAVE_API_OK;
        }

        OggCodec::OggCodec(DataSource *pSource)
            : AudioCodec(pSource),
            m_decoderExists(0)
        {

        }

        OggCodec::~OggCodec()
        {
            if (m_decoderExists == 1)
            {
                ov_clear(&m_ogg);
                m_decoderExists = 0;
            }
        }

        /**
        * @brief Initialise the decoder, prime for processing data
        */
        SceHwError OggCodec::init()
        {
            int32_t res = 0;
            ov_callbacks ioCb;
            vorbis_info *info;

            res = isOggFormat(m_dataSource);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            ioCb.read_func = OggRead;
            ioCb.seek_func = OggSeek;
            ioCb.tell_func = OggTell;
            ioCb.close_func = OggClose;

            res = ov_open_callbacks(m_dataSource, &m_ogg, NULL, 0, ioCb);
            if (res != 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_decoderExists = 1;

            info = ov_info(&m_ogg, -1);
            if (info == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            double dur = ov_time_total(&m_ogg, -1);
            if ((int)dur == OV_EINVAL)
            {
                dur = 0.0f;
            }

            m_numberOfChannels = info->channels;
            m_sampleRate = info->rate;
            m_nDuration = (int32_t)(dur * SECS_TO_MS);

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Seek within the audio data to the supplied milli-second offset.
        *
        * @param[in] nSeekOffset The milli-second offset.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError OggCodec::performSeek(int32_t nSeekOffset)
        {
            int32_t res = 0;

            if (m_decoderExists == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = ov_time_seek(&m_ogg, (double)nSeekOffset / SECS_TO_MS);
            if (res != 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Decode uSizeBytes as 16bit PCM data.
        *
        * @param[in] pBuffer Destination for the decoded audio data.
        * @param[in] uSizeBytes The size of pBuffer.
        * @param[out] pNumBytesReadOut The number of bytes written to pBuffer.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError OggCodec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
        {
            int32_t res = 0;
            int32_t readBytes = 0;
            int32_t currentSection = 0;
            char *buf = (char *)pBuffer;

            if (m_decoderExists == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pNumBytesReadOut = 0;

            while (uSizeBytes > 0)
            {
                res = ov_read(&m_ogg, buf, uSizeBytes, 0, 2, 1, &currentSection);

                if (res <= 0)
                {
                    *pNumBytesReadOut = readBytes;
                    return SCE_HEATWAVE_API_OK;
                }

                readBytes += res;
                uSizeBytes -= res;
                buf += res;
            }

            *pNumBytesReadOut = readBytes;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_OGG */
