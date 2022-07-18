/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synthchannelinterface.h
*
* @brief Bridge layer between HeatWave and the platform specific synth channel
*   Default behaviour is for ::updateParameters() to calculate surround parameters;
*   this can be overridden if direct values are required by the specific implementation
*
* @history
*   1.2 Remove unused eChannelType enumeration
*       Add InitParams structure
*       Change ::init() to accept InitParams* parameter
*       Change ::update to have a default implementation
*       Change parameter flush behaviour from immediate pass through to cache and then flush all parameters on ::update()
*       Remove unused member variables (m_pitch & m_dopplerShift)
*       Change maximum supported sample rate to 96kHz (kuMaxSampleRate)
*/

#ifndef _SCE_HEATWAVE_SYNTH_CHANNEL_INTERFACE_H_
#define _SCE_HEATWAVE_SYNTH_CHANNEL_INTERFACE_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"
#include <queue>
#include "heatwave_api_synthchannelsurroundparams.h"

namespace sce{
    namespace HeatWave{

        class PlatformSynth;
        class Panner;

        class SynthChannel : public MemoryHandler
        {
        public:

            typedef struct SynthBuffer : public MemoryHandler
            {
                void        *pData;
                uint32_t    uDataSizeBytes;
            } SynthBuffer;

            typedef struct InitParams : public MemoryHandler
            {
                int32_t nDestinationPort;
                int32_t nA3dPriority;
                uint32_t uNumChannels;
                uint32_t uSampleRate;
                bool bIsAdpcm;
            } InitParams;

            static SynthChannel* createChannel(int32_t nPortDestination);

            SynthChannel();
            virtual ~SynthChannel() {};

            SceHwError init(PlatformSynth *pSynth, InitParams *pParam);
            SceHwError release();

            /**
            * @brief Called from the SystemInteral update thread
            */
            virtual SceHwError update();

            SceHwError pushBuffer(SynthBuffer *pBuffer);
            SceHwError popBuffer(SynthBuffer **pOutBuffer);

            /**
            * @brief Commence output of audio data from current stream position
            */
            virtual SceHwError play() =0;

            /**
            * @brief Stop processing and rewind the stream to the beginning
            */
            virtual SceHwError stop() =0;

            /**
            * @brief Stop processing but do not alter the stream position
            */
            virtual SceHwError pause() =0;

            /**
            * @brief Get the current state of the SynthChannel as a SceHwSfxState
            */
            virtual SceHwError getState(SceHwSfxState *pOutState) =0;

            virtual SceHwError setPosition(SceHwVector vPosition);
            virtual SceHwError setVelocity(SceHwVector vVelocity);
            virtual SceHwError setForward(SceHwVector vForward);

            virtual SceHwError setRolloffDistances(float32_t fMinDistance, float32_t fMaxDistance);
            virtual SceHwError setDistanceFactor(float32_t fFactor);
            virtual SceHwError setEmitterCone(float32_t fInsideAng, float32_t fOutsideAng, float32_t fOutsideGain, float32_t fOutsideFreq);

            virtual SceHwError setGainMultiplier(float32_t fGain);
            virtual SceHwError setPitchMultiplier(float32_t fPitch);

            virtual SceHwError setAsRelative(bool bRelativeToListener);

            virtual SceHwError updateParameters();

            static const uint32_t kuMaxChannels = 2;
            static const uint32_t kuMaxSampleRate = 96000;

        protected:

            /**
            * @brief Set the output channel volumes
            */
            virtual SceHwError setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels) =0;

            /**
            * @brief Set the playback frequency multiplier
            */
            virtual SceHwError setPitch(float32_t fPitch) =0;

            /**
            * @brief Set the frequency of low pass cutoff filter
            *
            * @param fLowpassCutoffFactor 0 -> 1.0f.  1.0f is maximium frequency
            */
            virtual SceHwError setLowpassCutoff(float32_t fLowpassCutoffFactor) =0;

            virtual SceHwError initSynthChannel(PlatformSynth *pSynth, InitParams *pParam) =0;
            virtual SceHwError releaseSynthChannel() =0;

            void flagSurroundParamsAsDirty();

            std::queue<SynthBuffer*> m_buffersToProcess; ///< FIFO queue of SynthBuffer for the channel to play
            std::queue<SynthBuffer*> m_processedBuffers; ///< FIFO queue of SynthBuffers the channel has processed

            float32_t*  m_volumeMatrix;         ///< Final surround volume matrix
            uint32_t    m_numMatrixChannels;    ///< Number of channels in the matrix

            float32_t m_gainMultiplier; ///< User set gain
            float32_t m_pitchMultiplier; ///< User set playback frequency multiplier

            SynthChannelSurroundParams m_surroundParams;
            Panner* m_panner;

            bool m_areParamsDirty;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_SYNTH_CHANNEL_INTERFACE_H_ */
