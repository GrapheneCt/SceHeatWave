/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_audiofilestream.h"
#include "heatwave_api_errors.h"
#include <stdio.h>
#include "heatwave_api_datasource.h"
#include "heatwave_api_audiocodec.h"

namespace sce{
    namespace HeatWave{

        AudioFileStream::AudioFileStream()
            : AudioStream(),
            m_codec(NULL),
            m_dataSource(NULL)
        {
        }

        AudioFileStream::~AudioFileStream()
        {
            release();
        }

        /**
        * @brief Create a new AudioFileStream that is a copy of this instance
        */
        SceHwError AudioFileStream::clone(AudioStream **ppCloneStreamOut) const
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            AudioFileStream *cloneInstance = NULL;

            if (ppCloneStreamOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            cloneInstance = new AudioFileStream();
            if (cloneInstance == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            res = cloneInstance->init(m_dataSource, false);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                delete cloneInstance;
                return res;
            }

            *ppCloneStreamOut = cloneInstance;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Initialise the file ready to be read from (detect file type, setup data offsets, etc)
        */
        SceHwError AudioFileStream::init(const DataSource *pDataSource, bool bBasicInit)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (pDataSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = release();
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = pDataSource->clone(&m_dataSource); /* create an instance of pDataSource that this owns */
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            if (bBasicInit)
            {
                res = AudioCodec::validAudioCodec(m_dataSource); /* Use AudioCodec to validate data from the DataSource */
            }
            else
            {
                res = AudioCodec::createAudioCodec(m_dataSource, &m_codec); /* Use AudioCodec to get pcm data from the DataSource */
            }
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                release();
                return res;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError AudioFileStream::release()
        {
            if (isOpen())
            {
                m_codec->release();
                delete m_codec;
                m_codec = NULL;
            }

            if (m_dataSource != NULL)
            {
                m_dataSource->close();
                delete m_dataSource;
                m_dataSource = NULL;
            }

            return SCE_HEATWAVE_API_OK;
        }

        bool AudioFileStream::isOpen() const
        {
            return m_codec != NULL;
        }

        /**
        * @brief Reset the audio file stream.
        *
        * Prepare the file stream to produce audio data.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError AudioFileStream::reset()
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            const int32_t knStartOfFile = 0;

            if (!isOpen())
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            nRet = m_codec->seek(knStartOfFile);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief read from audio file into 16bit buffer
        */
        uint32_t AudioFileStream::read(int16_t *pBuffer, uint32_t uNumBytes)
        {
            uint32_t numBytesRead = 0;
            SceHwError res = SCE_HEATWAVE_API_OK;

            if (!isOpen())
            {
                return 0;
            }

            res = m_codec->read(pBuffer, uNumBytes, &numBytesRead);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return numBytesRead;
            }

            return numBytesRead;
        }

        /**
        * @brief Seek within the audio data to the supplied offset.
        *
        * @param[in] nSeekOffset The time to seek to in milli-seconds.
        */
        SceHwError AudioFileStream::seek(int32_t nSeekOffset)
        {
            if (!isOpen())
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            return m_codec->seek(nSeekOffset);
        }

        uint32_t AudioFileStream::getNumChannels() const
        {
            if (!isOpen())
            {
                return 0;
            }

            return m_codec->getNumberOfChannels();
        }

        uint32_t AudioFileStream::getSampleRate() const
        {
            if (!isOpen())
            {
                return 0;
            }

            return m_codec->getSampleRate();
        }

        /**
        * @brief Return the millisecond duration of the file.
        *
        * Query the codec for the playback duration in milliseconds.
        *
        * @return Duration in milliseconds.
        */
        int32_t AudioFileStream::getDuration() const
        {
            if (!isOpen())
            {
                return 0;
            }

            return m_codec->getDuration();
        }

        bool AudioFileStream::getIsAdpcm() const
        {
            if (!isOpen())
            {
                return 0;
            }

            return m_codec->getIsAdpcm();
        }

    } /* HeatWave */
} /* sce */
