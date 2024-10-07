/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_systeminternal.h"

using namespace sce::HeatWave;

SceHwError sceHeatWaveSoundCreateSource(SceHwSound hSoundHandle, SceHwSource *pOutSourceHandle)
{
    SoundInternal   *soundInternalHandle    = NULL;
    SourceInternal  *newSourceHandle        = NULL;

    if (pOutSourceHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    newSourceHandle = NULL;
    if (!soundInternalHandle->createSource(&newSourceHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }
    *pOutSourceHandle = _sceHeatWaveGetSourceHandle(newSourceHandle);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @note nIndex is the nth occupied source slot. Internally the array could have gaps.
*/
SceHwError sceHeatWaveSoundGetSource(SceHwSound hSoundHandle, int32_t nIndex, SceHwSource *pOutSource)
{
    SourceInternal  *sourceHandle = NULL;
    SoundInternal   *soundInternalHandle = NULL;
    uint32_t count = 0;

    if (pOutSource == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (nIndex < 0 || (uint32_t)nIndex >= _sceHeatWaveGetMaxSourceCount())
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if ((uint32_t)nIndex >= soundInternalHandle->getSourceCount())
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    while (nIndex >= 0 && count < _sceHeatWaveGetMaxSourceCount())
    {
        sourceHandle = soundInternalHandle->getChildSource(count);
        if (sourceHandle != NULL) /* Slot is occupied */
        {
            --nIndex;
        }
        count++;
    }

    if (sourceHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    *pOutSource = _sceHeatWaveGetSourceHandle(sourceHandle);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @brief The number of occupied source slots
*/
SceHwError sceHeatWaveSoundGetSourceCount(SceHwSound hSoundHandle, int32_t *pOutSourceCount)
{
    SoundInternal *soundInternalHandle = NULL;

    if (pOutSourceCount == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    *pOutSourceCount = soundInternalHandle->getSourceCount();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSoundCreateSfx(SceHwSound hSoundHandle, SceHwSfx *pOutSfxHandle)
{
    SoundInternal   *soundInternalHandle = NULL;
    SfxInternal     *newSfxHandle = NULL;

    if (pOutSfxHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (!soundInternalHandle->createSfx(&newSfxHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    *pOutSfxHandle = _sceHeatWaveGetSfxHandle(newSfxHandle);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @note nIndex is the nth occupied source slot. Internally the array could have gaps.
*/
SceHwError sceHeatWaveSoundGetSfx(SceHwSound hSoundHandle, int32_t nIndex, SceHwSfx *pOutSfxHandle)
{
    SoundInternal   *soundInternalHandle = NULL;
    SfxInternal     *sfxHandle = NULL;
    uint32_t count = 0;

    if (pOutSfxHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (nIndex < 0 || (uint32_t)nIndex >= _sceHeatWaveGetMaxSfxCount())
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if ((uint32_t)nIndex >= soundInternalHandle->getSfxCount())
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    while (nIndex >= 0 && count < _sceHeatWaveGetMaxSfxCount())
    {
        sfxHandle = soundInternalHandle->getSfxHandle(count);
        if (sfxHandle != NULL) /* Slot is occupied */
        {
            --nIndex;
        }
        count++;
    }

    if (sfxHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    *pOutSfxHandle = _sceHeatWaveGetSfxHandle(sfxHandle);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @brief The number of occupied sfx slots
*/
SceHwError sceHeatWaveSoundGetSfxCount(SceHwSound hSoundHandle, int32_t *pOutSfxCount)
{
    SoundInternal *soundInternalHandle = NULL;

    if (pOutSfxCount == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    *pOutSfxCount = soundInternalHandle->getSfxCount();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSoundGetInfo(SceHwSound hSoundHandle, SceHwSoundInfo* pOutInfo)
{
    SoundInternal *soundInternalHandle = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (pOutInfo == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    pOutInfo->uNumSfx = soundInternalHandle->getSfxCount();
    pOutInfo->uNumSources = soundInternalHandle->getSourceCount();

    nRet = soundInternalHandle->getDuration(&pOutInfo->nDurationMs);
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSoundSetLoopCount(SceHwSound hSoundHandle, int32_t nLoopCount)
{
    SoundInternal *pSoundInternalHandle = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (nLoopCount < knLoopInfinite)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    pSoundInternalHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (pSoundInternalHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    nRet = pSoundInternalHandle->setLoopCount(nLoopCount);

    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    return SCE_HEATWAVE_API_OK;
}
