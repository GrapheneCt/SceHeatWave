/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synthinterface.h
*
* @brief Bridge layer between HeatWave and the platform specific synth
* Internally updates and feeds data to the output, unless in loopback mode
*
* @history
*   1.2     Change PlatformSynth::createChannel() to accept SynthChannel::InitParams* parameter
*           Change PlatformSynth::createChannel() to use SynthChannel::createAudio3dChannel() when 3d Audio voices are requested
*           Add ::isLoopback() to query if synth is in loopback mode (i.e. render to user supplied buffer)
*           Add ::getAudio3dObjectCount() to query number of active Audio3d objects
*   1.4     Add ::isPortSupported().
*   1.44    Add ::getInfo().
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_INTERFACE_H_
#define _SCE_HEATWAVE_API_SYNTH_INTERFACE_H_

#include "heatwave_api_errors.h"
#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"
#include "heatwave_api_synthchannelinterface.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Structure containing information about the synth.
        */
        typedef struct PlatformSynthInfo
        {
            int32_t nSampleRate; ///< The sample rate of the audio output.
            int32_t nSamplesElapsed; ///< The number of samples passed to the audio output.
        } PlatformSynthInfo;

        class PlatformSynth : public MemoryHandler
        {
        public:
            static PlatformSynth* createSynth(); ///< Factory

            PlatformSynth();
            virtual ~PlatformSynth() {};

            SceHwError init(bool bLoopBack);
            SceHwError release();

            SceHwError createChannel(SynthChannel::InitParams *pParam, SynthChannel **pOutChannel);
            SceHwError destroyChannel(SynthChannel *pChannel);

            SceHwError render(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);
            SceHwError isLoopback(bool *pbIsInLoopbackMode) const;

            SceHwError getNumOutputChannels(uint32_t *pNumChannels);

            SceHwError getAudio3dObjectCount(uint32_t *pObjectCountOut);

            /**
            * @brief Is there an existing synth context created by either user or HeatWave
            */
            virtual SceHwError doesContextExist(bool *pOutExists) =0;

            bool isPortSupported(int32_t nPort);

            virtual SceHwError openUserPort(int32_t nPortId, int32_t nUserId);
            virtual SceHwError closeUserPort(int32_t nPortId);

            virtual SceHwError getInfo(PlatformSynthInfo *pInfoOut) const;

        protected:

            /**
            * @brief Setup
            */
            virtual SceHwError createContext() =0;

            /**
            * @brief Strip down and clean up
            */
            virtual SceHwError destroyContext() =0;

            /**
            * @brief Pull data from the synth
            */
            virtual SceHwError renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels) = 0;

            bool m_isInitialised;
            bool m_doesHeatWaveOwnContext;
            bool m_loopback;
            uint32_t m_numOutputChannels;
            uint32_t m_numAudio3dObjects;
            bool m_bPortSupported[SCE_HEATWAVE_API_OUTPUT_MAX];
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_INTERFACE_H_ */
