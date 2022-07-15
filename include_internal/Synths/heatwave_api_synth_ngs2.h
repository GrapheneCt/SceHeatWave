/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synth_ngs2.h
*
* @brief Ngs2 HeatWave synth implementation
*
* @note Default behaviour is to use a libAudioOut port for output.  SCE_HEATWAVE_API_USE_AUDIO3D must
*       be defined for the output port to be a libAudio3d port. libAudio3d is used to output audio as audio objects.
*
* @history
*   1.2     Add SCE_HEATWAVE_API_USE_AUDIO3D to specify libAudio3d as the output rather then libAudioOut
*           Add a rack of libAudio3d voices
*   1.4     Add support for output to multiple ports (i.e. Pad, Aux, Main).
*   1.41    Fix for casting int user port handles into uint main port handle types.
*           Change ::closePort() to return SCE_HEATWAVE_API_ERROR_PARAM if passed a NGS2OutputPort with an invalid hAudioOutPort.
*   1.43    Fix SceAudio3dOpenParameters not fully initialised (call sceAudio3dGetDefaultOpenParameters to fill out with defaults).
*   1.44    Add functionality to query the number of samples processed.
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_NGS2_H_
#define _SCE_HEATWAVE_API_SYNTH_NGS2_H_

#include "../heatwave_api_synthinterface.h"
#include <ngs2.h>
#include <kernel.h>

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D
    #include <audio3d.h>

    typedef SceAudio3dPortId SceHwMainOutputPortId; ///< Main output port handle type.
#else
    #include <audioout.h>

    typedef int32_t SceHwMainOutputPortId; ///< Main output port handle type.
#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */

typedef int32_t SceHwAudioOutPortId; ///< User ports obtained from libAudioOut.

namespace sce{
    namespace HeatWave{

        /**
        * @brief Container for output of data from a voice to a audioout port.
        */
        typedef struct NGS2OutputPort
        {
            int16_t nBuffer[SCE_NGS2_MAX_GRAIN_SAMPLES * SCE_NGS2_MAX_VOICE_CHANNELS]; ///< Buffer of audio data.
            SceHwAudioOutPortId hAudioOutPort; ///< The port that will be used for output.
            int32_t nDestination; ///< Destination identifier (SCE_HEATWAVE_API_OUTPUT_XXX).
            SceNgs2Handle hVoiceHandle; ///< Handle of the master voice to obtain data from.
        } NGS2OutputPort;

        class Ngs2Synth : public PlatformSynth
        {
        public:
            Ngs2Synth();
            virtual ~Ngs2Synth();

            virtual SceHwError doesContextExist(bool *pOutExists);

            static void* process(void *pArg);

            bool    killRenderThread();

            static const int32_t kNumChannels = 8; /* 7.1 */
            static const int32_t kNumSamples = 256;
            static const int32_t kSampleRate = 48000;

            SceHwError getNgs2SamplerVoice(int32_t eDestinationPort, SceNgs2Handle *pOutVoiceHandle);
            SceHwError releaseNgs2Voice(SceNgs2Handle hVoiceHandle, int32_t eDestinationPort);
            SceHwError patchNgs2VoiceToDestination(SceNgs2Handle hVoiceHandle, int32_t nDestinationPort);

            void IncAudio3dObjectCount();
            void DecAudio3dObjectCount();

            SceHwMainOutputPortId getOutputPortId();

            SceHwError openUserPort(int32_t nPortId, int32_t nUserId);
            SceHwError closeUserPort(int32_t nPortId);

            SceHwError getInfo(PlatformSynthInfo *pInfoOut) const;

        protected:
            virtual SceHwError createContext();
            virtual SceHwError destroyContext();
            virtual SceHwError renderContext(int16_t *pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);

            SceHwError renderPorts(int32_t nNumSamplesPerChannel, int32_t nNumChannels);

        private:
            SceHwError initAudio3d();
            SceHwError releaseAudio3d();

            SceHwError setupMasteringVoice(int32_t nIndex, NGS2OutputPort *pPort);
            SceHwError createMasteringRack(int32_t nNumVoices, SceNgs2BufferAllocator *pAllocator);
            SceHwError setOutputParam(NGS2OutputPort *pPort, SceAudioOutOutputParam *pParam);

            SceHwError outputAudioOut();
            SceHwError outputlibAudio3d();

            SceHwError getSamplerVoiceFromRack(SceNgs2Handle hRack, std::queue<int32_t> &rIdQueue, SceNgs2Handle *pHandleOut);

            int32_t createSamplerRack(SceNgs2BufferAllocator *pAllocator);
            int32_t createAudio3dSamplerRack(SceNgs2BufferAllocator *pAllocator);

            SceHwError openOutputPorts();
            SceHwError closeOutputPorts();

            SceHwError openPort(SceUserServiceUserId nUser, const int32_t nPortType, NGS2OutputPort *pPort);
            SceHwError closePort(NGS2OutputPort *pPort);
            SceHwError startNgs2();
            SceHwError stopNgs2();

            SceHwError startRenderThread();
            SceHwError stopRenderThread();

            SceNgs2Handle   m_ngs2System;
            SceNgs2Handle   m_ngs2MasterRack;
            SceNgs2Handle   m_ngs2SamplerRack; ///< Voices played as part of the libAudio3d 'bed'
            SceNgs2Handle   m_a3dSamplerRack; ///< Voices played as libAudio3d 'object'

            ScePthread  m_renderThread; ///< Thread used to render audio from the Synth and feed the SceHwOutputPortId port
            bool        m_isRenderThreadAborted; ///< Flag to stop execution of the render thread

            const int32_t knMaxNgs2SamplerVoices = 256;
            const int32_t knMaxA3dVoices = 64;

            std::queue<int32_t> m_ngs2AvailableVoiceId;
            std::queue<int32_t> m_a3dAvailableVoiceId;

            NGS2OutputPort m_outputs[SCE_HEATWAVE_API_OUTPUT_MAX];

            int32_t m_nSamplesProcessed;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_NGS2_H_ */
