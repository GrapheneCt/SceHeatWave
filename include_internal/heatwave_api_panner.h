/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_panner.h
*
* @brief Control the listener and perform doppler and surround matrix calculations
*
* @history
*   1.2 Refactor ::calculate() to use several worker functions.
*       Change ::getListenerRelativePosition() to account for the listener orientation.
*   1.4 Change ::calculate to ensure pointer is NULL checked prior to access.
*/

#ifndef _SCE_HEATWAVE_API_PANNER_H_
#define _SCE_HEATWAVE_API_PANNER_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class SynthChannelSurroundParams;

        class Panner : public MemoryHandler
        {
        public:
            typedef enum ///< Order of speaker gains in the volume matrix
            {
                Speaker_FL  = 0,
                Speaker_FR  = 1,
                Speaker_C   = 2,
                Speaker_LFE = 3,
                Speaker_SL  = 4,
                Speaker_SR  = 5,
                Speaker_BL  = 6,
                Speaker_BR  = 7,
                Speaker_MAX = 8
            } eSpeakerID;

            Panner();
            ~Panner();

            SceHwError setListenerPosition(SceHwVector vPosition);
            SceHwError setListenerVelocity(SceHwVector vVelocity);
            SceHwError setListenerOrientation(SceHwVector vForward, SceHwVector vUp);

            SceHwError setDopplerFactor(float32_t fFactor);
            SceHwError getDopplerFactor(float32_t *pfFactor);
            SceHwError setSpeedOfSound(float32_t fSpeed);
            SceHwError getSpeedOfSound(float32_t *pfSpeed);

            SceHwError setListenerGain(float32_t fGain);
            SceHwError getListenerGain(float32_t *pfGain);

            SceHwError setDistanceModel(int32_t nModel);
            SceHwError getDistanceModel(int32_t *pnModel);

            SceHwError calculate(SynthChannelSurroundParams* pParams, uint32_t uNumVolumes, float32_t *pVolumeMatrixOut, float32_t *pDopplerShiftOut, float32_t *pLowPassOut);

            SceHwError getListenerRelativePosition(SceHwVector vPosition, SceHwVector *pRelativePositionOut);
            SceHwError calcDistanceGain(SceHwVector vRelativePostion, float32_t fMinDistance, float32_t fMaxDistance, float32_t fDistanceFactor, float32_t *pDistanceGainOut);
            SceHwError calcDopplerShift(SceHwVector vRelativePosition, SceHwVector vVelocity, float32_t *pDopplerShiftOut);
            SceHwError calcCone(SceHwVector vRelativePosition, SceHwVector vForward, float32_t fInsideAngle, float32_t fOutsideAngle, float32_t fOutsideGain, float32_t fOutsideLowpass, float32_t *pGainOut, float32_t *pFilterLowpassOut);
            SceHwError calcSpeakerVolumes(SceHwVector vRelativePosition, float32_t *pVolumeMatrixOut, uint32_t uNumVolumes);

        private:
            SceHwVector m_position;
            SceHwVector m_velocity;
            SceHwVector m_forward;
            SceHwVector m_up;
            float32_t m_dopplerFactor;
            float32_t m_speedOfSound;
            float32_t m_gain;
            int32_t m_distanceModel;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_PANNER_H_ */
