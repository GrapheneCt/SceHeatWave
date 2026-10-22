/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "heatwave_api_audiostream_pcm.h"
#include "heatwave_api.h"
#include "heatwave_api_errors.h"

namespace sce{
    namespace HeatWave{

        AudioStreamPCM::AudioStreamPCM()
            : AudioStream(),
            m_pRenderFunction(NULL),
            m_pUserData(NULL)
        {
            m_info.pcBuffer         = NULL;
            m_info.nBufferSizeBytes = 0;
        }

        AudioStreamPCM::~AudioStreamPCM()
        {
            m_info.nBufferSizeBytes = 0;
            m_info.pcBuffer = NULL;

            m_pRenderFunction(&m_info, m_pUserData, eStreamRelease);
        }

        /**
        * @brief    Create a duplicate of this instance.
        *
        *           Create a new AudioStreamPCM and duplicate the members of this.
        *
        * @param    ppCloneStreamOut Receive the created instance.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError AudioStreamPCM::clone(AudioStream **ppCloneStreamOut) const
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            AudioStreamPCM *pCloneInstance = NULL;

            if (ppCloneStreamOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            pCloneInstance = new AudioStreamPCM();
            if (pCloneInstance == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
            }

            nRet = pCloneInstance->init(&m_param, m_pRenderFunction, m_pUserData);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                delete pCloneInstance;
                return nRet;
            }

            *ppCloneStreamOut = pCloneInstance;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Initialise to a state ready to produce audio data.
        *
        * Perform setup ready to begin producing audio data.
        *
        * @param pParam     Information about the audio data.
        * @param pFunc      User supplied callback function.
        * @param pUserData  User supplied data.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError AudioStreamPCM::init(const SceHwSourceRuntimeStreamParam* pParam, SceHwSourceRuntimeStreamCallbackFunction pFunc, void *pUserData)
        {
            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pFunc == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_param.nChannelCount = pParam->nChannelCount;
            m_param.nSampleRate = pParam->nSampleRate;

            m_pRenderFunction = pFunc;
            m_pUserData       = pUserData;

            m_info.pcBuffer = NULL;
            m_info.nBufferSizeBytes = 0;

            m_pRenderFunction(&m_info, m_pUserData, eStreamCreate);

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Reset the stream.
        *
        * Set the stream ready to produce audio data.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError AudioStreamPCM::reset()
        {
            if (m_pRenderFunction == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            m_pRenderFunction(&m_info, m_pUserData, eStreamReset);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Read a block of audio data from the runtime audio stream.
        *
        * Generate a block of audio data for playback.
        *
        * @param pnBuffer Memory buffer for audio data.
        * @param uSizeBytes Byte size of pnBuffer.
        *
        * @return The number of bytes of data successfully read.
        */
        uint32_t AudioStreamPCM::read(int16_t* pnBuffer, uint32_t uSizeBytes)
        {
            SceHwSourceRuntimeStreamCallbackInfo info;

            if (pnBuffer == NULL)
            {
                return 0;
            }
            if (uSizeBytes == 0)
            {
                return 0;
            }

            info.pcBuffer = (char*)pnBuffer;
            info.nBufferSizeBytes = uSizeBytes;
            info.nBytesWrittenOut = 0;

            m_pRenderFunction(&info, m_pUserData, eStreamRender);

            return info.nBytesWrittenOut;
        }

        /**
        * @brief Do not support seek of user supplied audio data.
        *
        * @param[in] nSeekOffset The new offset time from start in milli-seconds.
        *
        * @return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION.
        */
        SceHwError AudioStreamPCM::seek(int32_t nSeekOffset)
        {
            return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
        }

        /**
        * @brief The number of channels.
        *
        * The channel count of the audio data.
        *
        * @return The number of channels.
        */
        uint32_t AudioStreamPCM::getNumChannels() const
        {
            return m_param.nChannelCount;
        }

        /**
        * @brief The sample rate.
        *
        * The sample rate of the audio data.
        *
        * @return The sample rate.
        */
        uint32_t AudioStreamPCM::getSampleRate() const
        {
            return m_param.nSampleRate;
        }

        /**
        * @brief Return unknown duration.
        *
        * It is not possible to know the duration in this case as we simply callback 
        * for a new buffer of audio data without any further knowledge (the callback 
        * will indicate when the audio data is complete).  However, in this case 
        * the user should know the duration as they are responsible for providing 
        * the audio data.
        *
        * @return knDurationUnknown
        */
        int32_t AudioStreamPCM::getDuration() const
        {
            return knDurationUnknown;
        }

        bool AudioStreamPCM::getIsAdpcm() const
        {
            return false;
        }

    } /* HeatWave */
} /* sce */
