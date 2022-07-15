/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @history
*   1.2 Add optional SceHwSfxPlayParam parameter to sceHeatWaveSfxPlay
*
*   1.3 Change sceHeatWaveSfxPlay to initialise the internal handle before playback.
*       Change sceHeatWaveSfxPlay to pass a default play parameter if there are no user supplied parameter.
*       Change sceHeatWaveSfxStop to release the internal handle after stopping playback.
*/

#include "heatwave_api.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_systeminternal.h"

#if defined( _WINDOWS)
    #include <float.h>
    #define isfinite _finite
#else
    #include <math.h>
#endif /* _WINDOWS */

using namespace sce::HeatWave;

SceHwError sceHeatWaveSfxPlay(SceHwSfx hSfxHandle, SceHwSfxPlayParam *pParam /*= NULL*/)
{
    SfxInternal *sfxInternalHandle = NULL;
    int32_t nRet = SCE_HEATWAVE_API_OK;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (pParam != NULL && (pParam->bIsAudio3d || pParam->nDestination == SCE_HEATWAVE_API_OUTPUT_AUDIO3D))
    {
        if (pParam->nAudio3dPriority < knSfxAudio3dPriorityMax || pParam->nAudio3dPriority > knSfxAudio3dPriorityMin)
        {
            return SCE_HEATWAVE_API_ERROR_PARAM;
        }
    }

    if (pParam == NULL)
    {
        pParam = &SfxInternal::s_defaultPlaybackParam;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    nRet = sfxInternalHandle->initialise(pParam);
    if (SCE_HEATWAVE_API_SUCCEEDED(nRet))
    {
        nRet = sfxInternalHandle->play();
    }

    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSfxStop(SceHwSfx hSfxHandle)
{
    SfxInternal *pSfxInternalHandle = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    pSfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (pSfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    nRet = pSfxInternalHandle->stop();
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        SCE_HEATWAVE_UNLOCK();
        return nRet;
    }

    nRet = pSfxInternalHandle->release();

    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSfxPause(SceHwSfx hSfxHandle)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->pause();

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxResume(SceHwSfx hSfxHandle)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->resume();

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxSetVolume(SceHwSfx hSfxHandle, float32_t fVolume)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(fVolume) || fVolume < kfSfxVolumeMin || fVolume > kfSfxVolumeMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->setVolume(fVolume);

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxGetVolume(SceHwSfx hSfxHandle, float32_t *pOutVolume)
{
    SfxInternal *sfxInternalHandle = NULL;

    if (pOutVolume == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    *pOutVolume = sfxInternalHandle->getVolume();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSfxSetPitchMultiplier(SceHwSfx hSfxHandle, float32_t fPitchMultiplier)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(fPitchMultiplier) || fPitchMultiplier < kfSfxPitchMin || fPitchMultiplier > kfSfxPitchMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->setPitch(fPitchMultiplier);

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxGetPitchMultiplier(SceHwSfx hSfxHandle, float32_t *pOutPitchMultiplier)
{
    SfxInternal *sfxInternalHandle = NULL;

    if (pOutPitchMultiplier == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    *pOutPitchMultiplier = sfxInternalHandle->getPitch();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSfxGetState(SceHwSfx hSfxHandle, SceHwSfxState *pOutState)
{
    SfxInternal *sfxInternalHandle = NULL;

    if (pOutState == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    sfxInternalHandle->getState(pOutState);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSfxSetPosition(SceHwSfx hSfxHandle, SceHwVector vPosition)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(vPosition.fX) || !isfinite(vPosition.fY) || !isfinite(vPosition.fZ))
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->setPosition(vPosition);

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxSetVelocity(SceHwSfx hSfxHandle, SceHwVector vVelocity)
{
    SfxInternal *sfxInternalHandle      = NULL;
    bool success = false;
    float32_t sfxspeedSq = 0.0f;
    const float32_t kfSpeedOfSoundSq    = kfSpeedOfSound * kfSpeedOfSound;
    

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(vVelocity.fX) || !isfinite(vVelocity.fY) || !isfinite(vVelocity.fZ))
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    sfxspeedSq = (vVelocity.fX * vVelocity.fX) + (vVelocity.fY * vVelocity.fY) + (vVelocity.fZ * vVelocity.fZ);

    /* Clamp to speed of sound */
    if (sfxspeedSq >= kfSpeedOfSoundSq)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->setVelocity(vVelocity);

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxSetForward(SceHwSfx hSfxHandle, SceHwVector vForward)
{
    SfxInternal *sfxInternalHandle  = NULL;
    bool success = false;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(vForward.fX) || !isfinite(vForward.fY) || !isfinite(vForward.fZ))
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    success = sfxInternalHandle->setForward(vForward);

    SCE_HEATWAVE_UNLOCK();
    return success ? SCE_HEATWAVE_API_OK : SCE_HEATWAVE_API_ERROR;
}

SceHwError sceHeatWaveSfxSeek(SceHwSfx hSfxHandle, int32_t nTimeMs)
{
    SfxInternal *pSfxInternalHandle = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (nTimeMs < 0)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    pSfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (pSfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    nRet = pSfxInternalHandle->seek(nTimeMs);

    SCE_HEATWAVE_UNLOCK();
    return nRet;
}

SceHwError sceHeatWaveSfxGetInfo(SceHwSfx hSfxHandle, SceHwSfxInfo* pOutInfo)
{
    SfxInternal *sfxInternalHandle = NULL;

    if (pOutInfo == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sfxInternalHandle = _sceHeatWaveGetSfxInternal(hSfxHandle);
    if (sfxInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    sfxInternalHandle->getPosition(&pOutInfo->vPosition);
    sfxInternalHandle->getVeloctiy(&pOutInfo->vVelocity);
    sfxInternalHandle->getForward(&pOutInfo->vForward);

    sfxInternalHandle->getState(&pOutInfo->eSfxState);

    pOutInfo->hParentSound = _sceHeatWaveGetSoundHandle(sfxInternalHandle->getParentSound());

    pOutInfo->nTimeElapsedMs = sfxInternalHandle->getElapsedTime();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

