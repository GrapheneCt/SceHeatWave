/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_synthchannel_ngs2libaudio3d.h
*
* @brief Specialism of Ngs2SynthChannel for playback as a libAudio3d object
*
* @history
*   1.2 File added
*/

#ifndef _SCE_HEATWAVE_API_SYNTHCHANNEL_NGS2_LIBAUDIO3D_H_
#define _SCE_HEATWAVE_API_SYNTHCHANNEL_NGS2_LIBAUDIO3D_H_

#include "heatwave_api_synthchannel_ngs2.h"

namespace sce{
    namespace HeatWave{

        class NGS2LibAudio3dSynthChannel : public Ngs2SynthChannel
        {
        public:
            NGS2LibAudio3dSynthChannel();
            ~NGS2LibAudio3dSynthChannel();

            SceHwError setPitch(float32_t fPitch);

            SceHwError updateParameters();
            SceHwError setVolumeMatrix(float32_t *pVol, uint32_t uNumChannels);

        protected:
            SceHwError setupNgs2Voice(SceNgs2Handle hVoiceHandle, SceNgs2WaveformFormat *pWaveFormat, uint32_t uFlags);
            SceHwError setNgs2VoiceFilterByFcq(SceNgs2Handle hVoiceHandle, uint32_t uIndex, uint32_t uLocation, uint32_t uType, uint32_t uChannelMask, float fC, float fQ, float fLevel);
            SceHwError addNgs2VoiceWaveformBlocks(SceNgs2Handle hVoiceHandle, const void *pData, const SceNgs2WaveformBlock *pBlock, uint32_t uNumBlocks, uint32_t uFlags);
            SceHwError getNgs2VoiceDecodedDataSize(SceNgs2Handle hVoiceHandle, uint64_t *pDecodedDataSizeOut);
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTHCHANNEL_NGS2_LIBAUDIO3D_H_ */
