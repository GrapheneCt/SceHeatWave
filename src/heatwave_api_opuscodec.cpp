/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_OPUS

#include <opusfile.h>

#include "heatwave_api_opuscodec.h"

#include "heatwave_api_errors.h"
#include "heatwave_api_datasource.h"
#include "heatwave_api.h"

/**
* @brief Fixed sampling rate for OPUS codec.
*
*/
#define OPUS_SAMPLING_RATE (48000)

/**
* @brief Fixed channel count for OPUS codec.
*
*/
#define OPUS_CHANNEL_NUM (2)

/**
* @brief Multiplier to covert from seconds to milliseconds.
*
* 1sec == 1000mS
*/
#define SECS_TO_MS (1000.0f)

extern "C"
{
    void *opus_alloc(unsigned int size)
    {
        return sce::HeatWave::MemoryHandler::mallocCb(size);
    }

    void opus_free(void *ptr)
    {
        sce::HeatWave::MemoryHandler::freeCb(ptr);
    }

    void *opus_memcpy(void *dst, void *src, size_t size)
    {
        return sce::HeatWave::MemoryHandler::memcpyCb(dst, src, size);
    }

    void opus_memset(void *mem, int32_t val, size_t size)
    {
        sce::HeatWave::MemoryHandler::memsetCb(mem, val, size);
    }

    void *opus_memmove(void *dst, void *src, size_t size)
    {
        return sce::HeatWave::MemoryHandler::memmoveCb(dst, src, size);
    }
}

namespace sce{
    namespace HeatWave{

        int32_t OpusCodec::OpusRead(void *datasource, unsigned char *ptr, int32_t nbytes)
        {
            DataSource *pStream = (DataSource *)datasource;

            int32_t ret = pStream->read(ptr, nbytes, 1) * nbytes;
            if (ret == 0)
            {
                ret = EOF;
            }

            return ret;
        }

        int32_t OpusCodec::OpusSeek(void *datasource, opus_int64 offset, int32_t whence)
        {
            DataSource *pStream = (DataSource *)datasource;

            int32_t ret = pStream->seek((int32_t)offset, whence);
            if (ret > 0)
            {
                ret = 0;
            }

            return ret;
        }

        opus_int64 OpusCodec::OpusTell(void *datasource)
        {
            DataSource *pStream = (DataSource *)datasource;

            return pStream->tell();;
        }

        int32_t OpusCodec::OpusClose(void *datasource)
        {
            return 0;
        }

        /**
        * @brief Verify pStream is OPUS format
        */
        SceHwError OpusCodec::isOpusFormat(DataSource *pStream)
        {
            int32_t ret = 0;
            OggOpusFile *opus;
            OpusFileCallbacks ioCb;

            ioCb.read = OpusRead;
            ioCb.seek = OpusSeek;
            ioCb.tell = OpusTell;
            ioCb.close = OpusClose;

            pStream->seek(0, SEEK_SET);

            opus = op_test_callbacks(pStream, &ioCb, NULL, 0, &ret);
            if (ret != 0 || !opus)
            {
                return SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT;
            }

            op_free(opus);

            pStream->seek(0, SEEK_SET);

            return SCE_HEATWAVE_API_OK;
        }

        OpusCodec::OpusCodec(DataSource *pSource)
            : AudioCodec(pSource),
            m_opus(NULL),
            m_decoderExists(0)
        {

        }

        OpusCodec::~OpusCodec()
        {
            if (m_decoderExists == 1)
            {
                op_free(m_opus);
                m_decoderExists = 0;
            }
        }

        /**
        * @brief Initialise the decoder, prime for processing data
        */
        SceHwError OpusCodec::init()
        {
            int32_t res = 0;
            OpusFileCallbacks ioCb;

            res = isOpusFormat(m_dataSource);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            ioCb.read = OpusRead;
            ioCb.seek = OpusSeek;
            ioCb.tell = OpusTell;
            ioCb.close = OpusClose;

            m_opus = op_open_callbacks(m_dataSource, &ioCb, NULL, 0, &res);
            if (res != 0 || !m_opus)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_decoderExists = 1;

            int64_t dur = op_pcm_total(m_opus, -1);
            if (dur == OP_EINVAL)
            {
                dur = 0;
            }

            m_numberOfChannels = OPUS_CHANNEL_NUM;
            m_sampleRate = OPUS_SAMPLING_RATE;
            m_nDuration = (int32_t)((dur * SECS_TO_MS) / OPUS_SAMPLING_RATE);

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Seek within the audio data to the supplied milli-second offset.
        *
        * @param[in] nSeekOffset The milli-second offset.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError OpusCodec::performSeek(int32_t nSeekOffset)
        {
            int32_t res = 0;

            if (m_decoderExists == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            res = op_pcm_seek(m_opus, (int64_t)((nSeekOffset / SECS_TO_MS) * OPUS_SAMPLING_RATE));
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
        SceHwError OpusCodec::performRead(int16_t *pBuffer, uint32_t uSizeBytes, uint32_t *pNumBytesReadOut)
        {
            int32_t res = 0;
            int32_t readBytes = 0;
            int32_t currentSection = 0;
            char *buf = (char *)pBuffer;

            if (m_decoderExists == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }
            uint32_t tor = uSizeBytes;
            *pNumBytesReadOut = 0;

            while (uSizeBytes > 0)
            {
                res = op_read_stereo(m_opus, (opus_int16 *)buf, uSizeBytes / sizeof(opus_int16));
                if (res <= 0)
                {
                    *pNumBytesReadOut = readBytes;
                    return SCE_HEATWAVE_API_OK;
                }

                uint32_t rb = res * sizeof(opus_int16) * OPUS_CHANNEL_NUM;
                readBytes += rb;
                uSizeBytes -= rb;
                buf += rb;
            }

            *pNumBytesReadOut = readBytes;

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_OPUS */
