/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_synthchannelsurroundparams.h
*
* @brief POD containing the information required to perform surround calculations
*
* @history
*   1.2 Add nPriority : libAudio3d object priority
*/

#ifndef _SCE_HEATWAVE_API_SYNTH_CHANNEL_SURROUND_PARAMS_H_
#define _SCE_HEATWAVE_API_SYNTH_CHANNEL_SURROUND_PARAMS_H_

#include "heatwave_api_memory.h"
#include "heatwave_api_types.h"

namespace sce{
    namespace HeatWave{

        class SynthChannelSurroundParams : public MemoryHandler
        {
        public:
            SynthChannelSurroundParams();

            SceHwVector vPosition;          ///< world position
            SceHwVector vVelocity;          ///< world velocity (meters per second)
            SceHwVector vForward;           ///< world orientation
            float32_t   fMinDistance;       ///< distances above this have volume distance rolloff applied
            float32_t   fMaxDistance;       ///< distance above this cease to apply further volume distance rolloff
            float32_t   fInsideAngle;       ///< listener angle above this applies volume rolloff & high pass filter
            float32_t   fOutsideAngle;      ///< listener angle above this ceases to apply further volume rolloff & low pass filter
            float32_t   fOutsideGain;       ///< volume rolloff at fOutsideAngle
            float32_t   fOutsideFreq;       ///< low pass freq at fOutsideAngle
            bool        bListenerRelative;  ///< Is the position relative to the listener position
            int32_t     nPriority;          ///< Priority used for playback (libAudio3d)
            float32_t   fDistanceFactor;    ///< Distance volume attenuation factor
            bool		bIs2d;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYNTH_CHANNEL_SURROUND_PARAMS_H_ */
