/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_synthchannelsurroundparams.h"
#include "heatwave_api.h"

namespace sce{
    namespace HeatWave{

        static const float32_t kfDefaultMinDistance = 1.0f;
        static const float32_t kfDefaultMaxDistance = 10.0f;

        static const float32_t kfDefaultEmitterInside   =  90.0f;
        static const float32_t kfDefaultEmitterOutside  = 270.0f;
        static const float32_t kfDefaultEmitterVolume   =   1.0f;
        static const float32_t kfDefaultEmitterFreq     =   1.0f;

        SynthChannelSurroundParams::SynthChannelSurroundParams()
            : MemoryHandler()
        {
            vPosition.fX = 0.0f;
            vPosition.fY = 0.0f;
            vPosition.fZ = 0.0f;

            vVelocity.fX = 0.0f;
            vVelocity.fY = 0.0f;
            vVelocity.fZ = 0.0f;

            vForward.fX = 0.0f;
            vForward.fY = 0.0f;
            vForward.fZ = -1.0f;

            fMinDistance = kfDefaultMinDistance;
            fMaxDistance = kfDefaultMaxDistance;

            fInsideAngle    = kfDefaultEmitterInside;
            fOutsideAngle   = kfDefaultEmitterOutside;
            fOutsideGain    = kfDefaultEmitterVolume;
            fOutsideFreq    = kfDefaultEmitterFreq;

            bListenerRelative = false;
            bIs2d             = false;

            nPriority = knSfxAudio3dPriorityMin;

            fDistanceFactor = 1.0f;
        }

    } /* HeatWave */
} /* sce */
