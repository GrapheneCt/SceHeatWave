/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_panner.h"
#include "heatwave_api_errors.h"
#include <math.h>
#include <float.h>
#include "heatwave_api_synthchannelsurroundparams.h"
#include "heatwave_api.h"

namespace sce{
    namespace HeatWave{

        #define SCE_HEATWAVE_API_PI                 (3.14159265358979323846)
        #define SCE_HEATWAVE_API_2PI                (2.0 * SCE_HEATWAVE_API_PI)
        #define SCE_HEATWAVE_API_DEGREE_TO_RADIAN   (SCE_HEATWAVE_API_PI / 180.0)
        #define SCE_HEATWAVE_API_HALF_PI            (0.5 * SCE_HEATWAVE_API_PI)

        typedef struct _speakerinfo
        {
            float32_t angle;
            Panner::eSpeakerID id;
        } _speakerinfo;

        /**
        * @brief Make sure that the vector values are reasonable
        *
        * @return SCE_HEATWAVE_API_ERROR_PARAM if vVec is invalid
        */
        static SceHwError isVectorFinite(SceHwVector vVec)
        {
#ifdef _WINDOWS
            if (!_finite(vVec.fX))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (!_finite(vVec.fY))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (!_finite(vVec.fZ))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
#else
            if (!isfinite(vVec.fX))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (!isfinite(vVec.fY))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (!isfinite(vVec.fZ))
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
#endif /* _WINDOWS */
            return SCE_HEATWAVE_API_OK;
        }

        static void _normaliseVector(SceHwVector *pVector)
        {
            float32_t length = 0.0f;
            float32_t inverseLen = 1.0f;

            if (pVector == NULL)
            {
                return;
            }

            length = sqrtf(pVector->fX*pVector->fX + pVector->fY*pVector->fY + pVector->fZ*pVector->fZ);
            if (length == 0.0f)
            {
                return;
            }
            inverseLen = 1.0f / length;

            pVector->fX *= inverseLen;
            pVector->fY *= inverseLen;
            pVector->fZ *= inverseLen;
        }

        static float32_t _dotProduct(SceHwVector vIn1, SceHwVector vIn2)
        {
            const float32_t result = (vIn1.fX * vIn2.fX + vIn1.fY * vIn2.fY + vIn1.fZ * vIn2.fZ);
            return result;
        }

        /**
        * @brief Find which speaker pair on unit circle fAngle is between
        *
        * @param pSpeaker1Out   Id of the speaker to the left of fAngle
        * @param pSpeaker2Out   Id of the speaker to the right of fAngle
        * @param pInterpolateOut    Distance along pSpeaker1Out to pSpeaker2Out that fAngle is at (0,1)
        *
        * @note kbPanToCenter specifies whether center speaker is used for panning
        */
        static SceHwError _getSpeakerPair(const float32_t fAngle, Panner::eSpeakerID *pSpeakerOut1, Panner::eSpeakerID *pSpeakerOut2, float32_t *pInterpolateOut)
        {
            /* Speaker angles on unit circle in radians in clockwise order */
            /* http://www.dolby.com/us/en/guide/surround-sound-speaker-setup/7-1-setup.html */
            const _speakerinfo ksSpeakersWithCenter[] = {
                { (float32_t)(0.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_C },
                { (float32_t)(30.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FR },
                { (float32_t)(90.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_SR },
                { (float32_t)(135.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_BR },
                { (float32_t)(225.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_BL },
                { (float32_t)(270.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_SL },
                { (float32_t)(330.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FL },
                { (float32_t)(360.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_C }
            };

            const _speakerinfo ksSpeakersNoCenter[] = {
                { (float32_t)(-30.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FL },
                { (float32_t)(30.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FR },
                { (float32_t)(90.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_SR },
                { (float32_t)(135.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_BR },
                { (float32_t)(225.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_BL },
                { (float32_t)(270.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_SL },
                { (float32_t)(330.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FL },
                { (float32_t)(390.0  * SCE_HEATWAVE_API_DEGREE_TO_RADIAN), Panner::Speaker_FR },
            };

            int32_t numberOfSpeakers = 0;
            const _speakerinfo *ksSpeakers = NULL;
            const _speakerinfo *pSpeakerLeft = NULL; ///< Speaker to the left of the emission point
            const _speakerinfo *pSpeakerRight = NULL; ///< Speaker to the right of the emission point

            const bool kbPanToCenter = false; ///< Ignore the center speaker for panning

            if (fAngle < 0.0f || fAngle >(float32_t)SCE_HEATWAVE_API_2PI)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pSpeakerOut1 == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pSpeakerOut2 == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (kbPanToCenter)
            {
                ksSpeakers = ksSpeakersWithCenter;
                numberOfSpeakers = (int32_t)(sizeof(ksSpeakersWithCenter) / sizeof(_speakerinfo));
            }
            else
            {
                ksSpeakers = ksSpeakersNoCenter;
                numberOfSpeakers = (int32_t)(sizeof(ksSpeakersNoCenter) / sizeof(_speakerinfo));
            }

            if (ksSpeakers == NULL || numberOfSpeakers == 0)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            pSpeakerLeft = &ksSpeakers[0]; ///< Speaker to the left of the emission point
            pSpeakerRight = &ksSpeakers[1]; ///< Speaker to the right of the emission point

            for (int32_t i = 0; i < numberOfSpeakers - 1; i++)
            {
                if (fAngle >= pSpeakerLeft->angle && fAngle < pSpeakerRight->angle)
                {
                    break;
                }

                pSpeakerRight++;
                pSpeakerLeft++;
            }

            *pSpeakerOut1 = pSpeakerLeft->id;
            *pSpeakerOut2 = pSpeakerRight->id;

            *pInterpolateOut = (fAngle - pSpeakerLeft->angle) / (pSpeakerRight->angle - pSpeakerLeft->angle); /* distance between the speaker pair 'coneAngle' is at */

            if (*pInterpolateOut  < 0.0f || *pInterpolateOut > 1.0f)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Downmix the pVolume 7 channel matrix to pOutputMatrix
        */
        static SceHwError _performFoldown(const float *pVoumes, float32_t *pOutputMatrix, const int32_t nTargetChannels)
        {
            const float32_t kfDefaultLFEVol = 0.0f; //< Don't send to LFE by default

            if (pVoumes == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pOutputMatrix == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (nTargetChannels == 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (nTargetChannels == 2) /* 2.0 */
            {
                /* LEFT = 1.0 x L + 0.707 x C + 0.707 x Ls + 0.707 x Le */
                pOutputMatrix[Panner::Speaker_FL] = (float32_t)(pVoumes[Panner::Speaker_FL]
                    + (0.707 * pVoumes[Panner::Speaker_C])
                    + (0.707 * pVoumes[Panner::Speaker_SL])
                    + (0.707 * pVoumes[Panner::Speaker_BL]));

                /* RIGHT = 1.0 x R + 0.707 x C + 0.707 x Rs + 0.707 x Re */
                pOutputMatrix[Panner::Speaker_FR] = (float32_t)(pVoumes[Panner::Speaker_FR]
                    + (0.707 * pVoumes[Panner::Speaker_C])
                    + (0.707 * pVoumes[Panner::Speaker_SR])
                    + (0.707 * pVoumes[Panner::Speaker_BR]));
            }
            else if (nTargetChannels == 8) /* 7.1 */
            {
                pOutputMatrix[Panner::Speaker_FL] = pVoumes[Panner::Speaker_FL];
                pOutputMatrix[Panner::Speaker_FR] = pVoumes[Panner::Speaker_FR];
                pOutputMatrix[Panner::Speaker_C] = pVoumes[Panner::Speaker_C];

                pOutputMatrix[Panner::Speaker_LFE] = kfDefaultLFEVol;

                pOutputMatrix[Panner::Speaker_SL] = pVoumes[Panner::Speaker_SL];
                pOutputMatrix[Panner::Speaker_SR] = pVoumes[Panner::Speaker_SR];

                pOutputMatrix[Panner::Speaker_BL] = pVoumes[Panner::Speaker_BL];
                pOutputMatrix[Panner::Speaker_BR] = pVoumes[Panner::Speaker_BR];
            }
            else if (nTargetChannels == 6) /* 5.1 */
            {
                pOutputMatrix[Panner::Speaker_FL] = pVoumes[Panner::Speaker_FL];
                pOutputMatrix[Panner::Speaker_FR] = pVoumes[Panner::Speaker_FR];
                pOutputMatrix[Panner::Speaker_C] = pVoumes[Panner::Speaker_C];

                pOutputMatrix[Panner::Speaker_LFE] = kfDefaultLFEVol;

                pOutputMatrix[Panner::Speaker_SL] = pVoumes[Panner::Speaker_SL] + (0.707f * pVoumes[Panner::Speaker_BL]);
                pOutputMatrix[Panner::Speaker_SR] = pVoumes[Panner::Speaker_SR] + (0.707f * pVoumes[Panner::Speaker_BR]);
            }
            else /* Unrecognised format */
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            return SCE_HEATWAVE_API_OK;
        }

        Panner::Panner()
        {
            m_position.fX = 0.0f;
            m_position.fY = 0.0f;
            m_position.fZ = 0.0f;

            m_velocity.fX = 0.0f;
            m_velocity.fY = 0.0f;
            m_velocity.fZ = 0.0f;

            m_forward.fX = 0.0f;
            m_forward.fY = 0.0f;
            m_forward.fZ = -1.0f;

            m_up.fX = 0.0f;
            m_up.fY = 1.0f;
            m_up.fZ = 0.0f;
        }

        Panner::~Panner()
        {
        }

        /**
        * @brief Check the supplied vector & pass onto the implementation specific ::setPosition() function
        */
        SceHwError Panner::setListenerPosition(SceHwVector vPosition)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = isVectorFinite(vPosition);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            m_position.fX = vPosition.fX;
            m_position.fY = vPosition.fY;
            m_position.fZ = vPosition.fZ;

            return res;
        }

        /**
        * @brief Check the supplied vector & pass onto the implementation specific ::setVelocity() function
        */
        SceHwError Panner::setListenerVelocity(SceHwVector vVelocity)
        {
            SceHwError  res = SCE_HEATWAVE_API_OK;
            float32_t   listenerspeedSq = 0.0f;

            const float32_t kSpeedOfSoundSq = kfSpeedOfSound * kfSpeedOfSound;

            res = isVectorFinite(vVelocity);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Clamp to velocity to the speed of sound */
            listenerspeedSq = (vVelocity.fX * vVelocity.fX) + (vVelocity.fY * vVelocity.fY) + (vVelocity.fZ * vVelocity.fZ);
            if (listenerspeedSq > kSpeedOfSoundSq)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_velocity.fX = vVelocity.fX;
            m_velocity.fY = vVelocity.fY;
            m_velocity.fZ = vVelocity.fZ;

            return res;
        }

        /**
        * @brief Check the supplied vectors & pass onto the implementation specific ::setOrientation() function
        */
        SceHwError Panner::setListenerOrientation(SceHwVector vForward, SceHwVector vUp)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            float forwardnorm = 1.0f;
            float upnorm = 1.0f;

            res = isVectorFinite(vForward);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            res = isVectorFinite(vUp);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            forwardnorm = sqrtf((vForward.fX * vForward.fX) + (vForward.fY * vForward.fY) + (vForward.fZ * vForward.fZ));
            upnorm = sqrtf((vUp.fX * vUp.fX) + (vUp.fY * vUp.fY) + (vUp.fZ * vUp.fZ));

            if (forwardnorm < FLT_EPSILON && forwardnorm > -FLT_EPSILON) /* Check that the vector is not zero length */
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            vForward.fX = vForward.fX / forwardnorm; /* normalise */
            vForward.fY = vForward.fY / forwardnorm;
            vForward.fZ = vForward.fZ / forwardnorm;

            if (upnorm < FLT_EPSILON && upnorm > -FLT_EPSILON) /* Check that the vector is not zero length */
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            vUp.fX = vUp.fX / upnorm; /* normalise */
            vUp.fY = vUp.fY / upnorm;
            vUp.fZ = vUp.fZ / upnorm;

            m_up.fX = vUp.fX;
            m_up.fY = vUp.fY;
            m_up.fZ = vUp.fZ;

            m_forward.fX = vForward.fX;
            m_forward.fY = vForward.fY;
            m_forward.fZ = vForward.fZ;

            return res;
        }

        /**
        * @brief Verify the parameters.  Calculate the speaker volumes, doppler and cone effects
        */
        SceHwError Panner::calculate(SynthChannelSurroundParams* pParams, uint32_t uNumVolumes, float32_t *pVolumeMatrixOut, float32_t *pDopplerShiftOut, float32_t *pLowPassOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;

            SceHwVector position    = { 0.0f, 0.0f, 0.0f };
            SceHwVector velocity    = { 0.0f, 0.0f, 0.0f };
            SceHwVector direction   = { 0.0f, 0.0f, 0.0f };

            float32_t flAttenuation = 1.0f;

            float32_t coneVolume = 1.0f;
            float32_t coneLowpass = 1.0f;
            float32_t dopplerShift = 1.0f;

            float32_t gain = 0.0f;

            /* Check the parameters */
            if (pParams == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            res = isVectorFinite(pParams->vPosition);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = isVectorFinite(pParams->vVelocity);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            res = isVectorFinite(pParams->vForward);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            position.fX = pParams->vPosition.fX;
            position.fY = pParams->vPosition.fY;
            position.fZ = pParams->vPosition.fZ;

            velocity.fX = pParams->vVelocity.fX;
            velocity.fY = pParams->vVelocity.fY;
            velocity.fZ = pParams->vVelocity.fZ;

            direction.fX = pParams->vForward.fX;
            direction.fY = pParams->vForward.fY;
            direction.fZ = pParams->vForward.fZ;

            if (pParams->fMinDistance < 0.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pParams->fMaxDistance < pParams->fMinDistance)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (uNumVolumes == 0 || uNumVolumes > Speaker_MAX)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pVolumeMatrixOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (pDopplerShiftOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            /* end of parameter checks! */

            /* translate to listener relative position */
            if (!pParams->bListenerRelative)
            {
                getListenerRelativePosition(pParams->vPosition, &position);
            }

            /* Distance gain rolloff */
            res = calcDistanceGain(position, pParams->fMinDistance, pParams->fMaxDistance, &flAttenuation);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Doppler */
            res = calcDopplerShift(position, velocity, &dopplerShift);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Apply the sound cone */
            res = calcCone(position, direction, pParams->fInsideAngle, pParams->fOutsideAngle, pParams->fOutsideGain, pParams->fOutsideFreq, &coneVolume, &coneLowpass);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Pan */
            res = calcSpeakerVolumes(position, pVolumeMatrixOut, uNumVolumes);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            /* Apply distance and cone attenuation */
            gain = 1.0f * flAttenuation;
            gain *= coneVolume;

            /* Fill out the volume matrix */
            for (uint32_t i = 0; i < uNumVolumes; i++)
            {
                pVolumeMatrixOut[i] *= gain;
            }

            *pDopplerShiftOut = dopplerShift;
            *pLowPassOut = coneLowpass;

            return res;
        }

        /**
        * @brief        Translate vPosition from world to listener relative
        *
        *               Takes the listener orientation and position into account 
        *               to return a position using the current world position and 
        *               orientation as the origin and axis.
        *
        * @param[in]    vPosition               World position to be translated.
        * @param[out]   pvRelativePositionOut   Result of the calculation.
        *
        * @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Panner::getListenerRelativePosition(SceHwVector vPosition, SceHwVector *pvRelativePositionOut)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            SceHwVector vRelativePosition = { 0.0f, 0.0f, 0.0f };
            SceHwVector vRight = { 0.0f, 0.0f, 0.0f };
            SceHwVector vAlignedPosition = { 0.0f, 0.0f, 0.0f };
            SceHwVector vLookAt = { 0.0f, 0.0f, 0.0f };

            /* Param validation */
            nRet = isVectorFinite(vPosition);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }
            if (pvRelativePositionOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* translate from world to local position */
            vRelativePosition.fX = vPosition.fX - m_position.fX;
            vRelativePosition.fY = vPosition.fY - m_position.fY;
            vRelativePosition.fZ = vPosition.fZ - m_position.fZ;

            /* Align the axis to the listener orientation */
            vRight.fX = (m_forward.fY * m_up.fZ) - (m_forward.fZ * m_up.fY); /* infer the right axis from up & forward */
            vRight.fY = (m_forward.fZ * m_up.fX) - (m_forward.fX * m_up.fZ);
            vRight.fZ = (m_forward.fX * m_up.fY) - (m_forward.fY * m_up.fX);

            vAlignedPosition.fX = _dotProduct(vRelativePosition, vRight);
            vAlignedPosition.fY = _dotProduct(vRelativePosition, m_up);

            /* apply the axis rotation to the position the sound emits from */
            vLookAt.fX = -m_forward.fX;
            vLookAt.fY = -m_forward.fY;
            vLookAt.fZ = -m_forward.fZ;

            vAlignedPosition.fZ = _dotProduct(vRelativePosition, vLookAt);


            pvRelativePositionOut->fX = vAlignedPosition.fX;
            pvRelativePositionOut->fY = vAlignedPosition.fY;
            pvRelativePositionOut->fZ = vAlignedPosition.fZ;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate the inverse square attenuation for distances in the range [fMinDistance, fMaxDistance]
        */
        SceHwError Panner::calcDistanceGain(SceHwVector vRelativePostion, float32_t fMinDistance, float32_t fMaxDistance, float32_t *pDistanceGainOut)
        {
            float32_t distance = 0.0f;
            SceHwError res = SCE_HEATWAVE_API_OK;

            res = isVectorFinite(vRelativePostion);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            if (fMinDistance < 0.0f)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (fMaxDistance < fMinDistance)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (pDistanceGainOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* Distance rolloff - clamped inverse square */
            distance = sqrtf(_dotProduct(vRelativePostion, vRelativePostion));

            if (distance < fMinDistance) /* clamp */
            {
                distance = fMinDistance;
            }
            if (distance > fMaxDistance)
            {
                distance = fMaxDistance;
            }

            *pDistanceGainOut = fMinDistance / distance;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate the doppler pitch shift
        */
        SceHwError Panner::calcDopplerShift(SceHwVector vRelativePosition, SceHwVector vVelocity, float32_t *pDopplerShiftOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            float32_t fVr = 0.0f;
            float32_t fVs = 0.0f;
            SceHwVector sourceToListener = { -vRelativePosition.fX, -vRelativePosition.fY, -vRelativePosition.fZ }; ///< Direction of movement

            res = isVectorFinite(vRelativePosition);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            res = isVectorFinite(vVelocity);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            if (pDopplerShiftOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            _normaliseVector(&sourceToListener);

            /* Doppler */
            fVr = _dotProduct(m_velocity, sourceToListener);
            fVs = _dotProduct(vVelocity, sourceToListener);

            *pDopplerShiftOut = (kfSpeedOfSound - fVr) / (kfSpeedOfSound - fVs); /* Negated doppler calculation due to right hand coordinate system */

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate the cone attenuation and lowpass filter
        */
        SceHwError Panner::calcCone(SceHwVector vRelativePosition, SceHwVector vForward, float32_t fInsideAngle, float32_t fOutsideAngle, float32_t fOutsideGain, float32_t fOutsideLowpass, float32_t *pGainOut, float32_t *pFilterLowpassOut)
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            SceHwVector sourceToListener = { -vRelativePosition.fX, -vRelativePosition.fY, -vRelativePosition.fZ };
            float32_t emitterDotProd = 0.0f;
            float32_t coneAngle = 0.0f;
            float32_t coneVolume = 0.0f;
            float32_t coneLowpass = 0.0f;
            
            res = isVectorFinite(vRelativePosition);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }
            res = isVectorFinite(vForward);
            if (SCE_HEATWAVE_API_FAILED(res))
            {
                return res;
            }

            _normaliseVector(&vForward);
            _normaliseVector(&sourceToListener);

            /* Calculate the angle between vForward and sourceToListener  */
            emitterDotProd = _dotProduct(vForward, sourceToListener);
            if (emitterDotProd > -FLT_EPSILON && emitterDotProd < FLT_EPSILON) /* emitter and listener are at the same position */
            {
                coneAngle = 0;
            }
            else
            {
                coneAngle = (float32_t)((180.0 / SCE_HEATWAVE_API_PI) * acos(emitterDotProd));
            }

            if (coneAngle >= fInsideAngle && coneAngle <= fOutsideAngle)
            {
                const float32_t coneAngleInterpolated = (coneAngle - fInsideAngle) / (fOutsideAngle - fInsideAngle);

                coneVolume = 1.0f + ((fOutsideGain - 1.0f) * coneAngleInterpolated);
                coneLowpass = 1.0f + ((fOutsideLowpass - 1.0f) * coneAngleInterpolated);
            }
            else if (coneAngle > fOutsideAngle) /* Clamp to max angle */
            {
                coneVolume = 1.0f + (fOutsideGain - 1.0f);
                coneLowpass = 1.0f + (fOutsideLowpass - 1.0f);
            }
            else
            {
                coneVolume = 1.0f;
                coneLowpass = 1.0f;
            }

            *pGainOut = coneVolume;
            *pFilterLowpassOut = coneLowpass;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief    Calculate the surround pan for a sound emitting at vRelativePosition
        *
        *           The amplitudes  for a 7 speaker configuration to pan a sound to vRelativePostion 
        *           are calculated (constant power pan). A folddown is then performed if needed to match 
        *           the user supplied speaker configuration.
        *
        * @param[in]    vRelativePosition   Position of the audio source relative to origin at the centre of the speakers.
        * @param[out]   pfVolumeMatrixOut   Array of floats representing the resultant gains of the speakers.
        * @param[in]    uNumVolumes         Number of entries in pfVolumeMatrixOut
        *
        * @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError Panner::calcSpeakerVolumes(SceHwVector vRelativePosition, float32_t *pfVolumeMatrixOut, uint32_t uNumVolumes)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            SceHwVector vPosition = { vRelativePosition.fX, vRelativePosition.fY, vRelativePosition.fZ };
            eSpeakerID eSpeaker1 = Speaker_MAX;
            eSpeakerID eSpeaker2 = Speaker_MAX;
            float32_t fEmitterAngle = 0.0f;
            float32_t fInterpolate = 0.0f;
            float32_t afSpeakerGains[Speaker_MAX] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

            /* param validation */
            nRet = isVectorFinite(vPosition);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }
            if (pfVolumeMatrixOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (uNumVolumes == 0 || uNumVolumes > Speaker_MAX)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            /* convert to unit vector */
            _normaliseVector(&vPosition);

            if (_dotProduct(vPosition, vPosition) == 0.0f) /* Sound is at exactly same position as the listener, emit from the front (not the rear) */
            {
                fEmitterAngle = 0.0f;
            }
            else
            {
                fEmitterAngle = atan2f(vRelativePosition.fX, -vRelativePosition.fZ);
            }

            if (fEmitterAngle < 0.0f) /* wrap */
            {
                fEmitterAngle += (float32_t)SCE_HEATWAVE_API_2PI;
            }

            /* find the speaker pair the sound will emit from */
            nRet = _getSpeakerPair(fEmitterAngle, &eSpeaker1, &eSpeaker2, &fInterpolate);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* Constant power pan between the 2 speakers */
            afSpeakerGains[eSpeaker1] = (float32_t)cos(fInterpolate * SCE_HEATWAVE_API_HALF_PI);
            afSpeakerGains[eSpeaker2] = (float32_t)sin(fInterpolate * SCE_HEATWAVE_API_HALF_PI);

            /* convert from 7.1 to the destination layout */
            nRet = _performFoldown(afSpeakerGains, pfVolumeMatrixOut, uNumVolumes);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
