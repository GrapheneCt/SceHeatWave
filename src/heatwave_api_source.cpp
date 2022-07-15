/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @history
*   1.2 Add sceHeatWaveSourceSetVolume(), sceHeatWaveSourceSetPitch()
        Change sceHeatWaveSourceGetInfo() to set the volume and pitch
*   1.3 Add sceHeatWaveSourceLoadRuntimeStream().
*/

#include "heatwave_api.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_audiofilestream.h"
#include "heatwave_api_audiostream_pcm.h"
#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_datasource.h"
#include "heatwave_api_systeminternal.h"

#if defined _WINDOWS
    #include <float.h>
    #define isfinite _finite
#else
    #include <math.h>
#endif /* _WINDOWS */

using namespace sce::HeatWave;

/**
* @brief Set hSourceHandle to use the contents of audio file at strFilePath
* @note AudioFileInternal will check that the file is a valid codec
*/
SceHwError sceHeatWaveSourceLoadFile(SceHwSource hSourceHandle, const char *strFilePath)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SourceInternal  *sourceInternal = NULL;
    SoundInternal   *parentSound    = NULL;
    SfxInternal     *sfxHandle      = NULL;
    FileSource      *filesource     = NULL;
    AudioFileStream *audioFile    = NULL;

    if (strFilePath == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    parentSound = sourceInternal->getParentSound();
    if (parentSound == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    /* stop any sfx that could be pointing at the source we are about to update */
    for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
    {
        sfxHandle = parentSound->getSfxHandle(i);
        if (sfxHandle != NULL)
        {
            sfxHandle->stop();
        }
    }

    sourceInternal->release(); /* Clean up (this might not be the first load) */

    /* The source is an audio file (wav for example) */
    audioFile = sourceInternal->init<AudioFileStream>();
    if (audioFile == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    /* The actual audio data is coming from a file */
    filesource = new FileSource();
    if (filesource == NULL)
    {
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    /* open the file (does it actually exist!?) */
    if (!filesource->open(strFilePath))
    {
        delete filesource;
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_FILE;
    }

    /* Check that the file is a format we can read, setup ready to read */
    res = audioFile->init(filesource);
    delete filesource;

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return res;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @brief Use the memory location from pMemoryFile as an audio file
*/
SceHwError sceHeatWaveSourceLoadMemory(SceHwSource hSourceHandle, SceHwMemoryFileInfo *pMemoryFile)
{
    SceHwError res = SCE_HEATWAVE_API_ERROR;
    SourceInternal  *sourceInternal = NULL;
    SoundInternal   *parentSound    = NULL;
    SfxInternal     *sfxHandle      = NULL;
    MemorySource    *memsource      = NULL;
    AudioFileStream *audioFile    = NULL;

    if (pMemoryFile == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (pMemoryFile->pBuffer == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (pMemoryFile->uBufferLen == 0)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    /* stop any sfx that could be pointing at the source we are about to update */
    parentSound = sourceInternal->getParentSound();
    if (parentSound == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
    {
        sfxHandle = parentSound->getSfxHandle(i);
        if (sfxHandle != NULL)
        {
            sfxHandle->stop();
        }
    }

    sourceInternal->release(); /* Clean up the source (this might not be the first load) */

    audioFile = sourceInternal->init<AudioFileStream>(); /* Initialise as an audio file */
    if (audioFile == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    /* The actual data is located in memory */
    memsource =  new MemorySource();
    if (memsource == NULL)
    {
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    /* Point to the provided memory location */
    if (!memsource->open(pMemoryFile->pBuffer, pMemoryFile->uBufferLen))
    {
        delete memsource;
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_FILE;
    }

    /* Detect the type of the audio data (wav?) and setup ready to read */
    res = audioFile->init(memsource);
    delete memsource;

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        sourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return res;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceLoadRuntimeStream(SceHwSource hSourceHandle, SceHwSourceRuntimeStreamParam* pParam, SceHwSourceRuntimeStreamCallbackFunction pFunc, void *pUserData)
{
    SceHwError nRet = SCE_HEATWAVE_API_OK;
    SourceInternal *pSourceInternal = NULL;
    SoundInternal *pParentSound = NULL;
    SfxInternal *pSfxInternal = NULL;
    AudioStreamPCM *pAudioStream = NULL;

    if (pParam == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (pFunc == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    pSourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (pSourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    /* stop any sfx that could be pointing at the source we are about to update */
    pParentSound = pSourceInternal->getParentSound();
    if (pParentSound == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
    {
        pSfxInternal = pParentSound->getSfxHandle(i);
        if (pSfxInternal != NULL)
        {
            pSfxInternal->stop();
        }
    }

    pSourceInternal->release(); /* Clean up the source (this might not be the first load) */

    pAudioStream = pSourceInternal->init<AudioStreamPCM>(); /* Initialise as a user supplied stream of PCM data */
    if (pAudioStream == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    nRet = pAudioStream->init(pParam, pFunc, pUserData);
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        pSourceInternal->release();
        SCE_HEATWAVE_UNLOCK();
        return nRet;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetType(SceHwSource hSourceHandle, SceHwSourceType eType)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (eType != eSource2d && eType != eSource3d)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    sourceInternal->type() = eType;

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetLoopCount(SceHwSource hSourceHandle, int32_t nLoopCount)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (nLoopCount < 0 && nLoopCount != knLoopInfinite)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    sourceInternal->loopCount() = nLoopCount;

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetRolloffDistances(SceHwSource hSourceHandle, float32_t fMinDistance, float32_t fMaxDistance)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(fMinDistance) || !isfinite(fMaxDistance))
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fMinDistance > fMaxDistance)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fMinDistance < 0.0f)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (sourceInternal->type() != eSource3d)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    sourceInternal->minDistance() = fMinDistance;
    sourceInternal->maxDistance() = fMaxDistance;

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetCone(SceHwSource hSourceHandle, float32_t fInsideAngle, float32_t fOutsideAngle, float32_t fOutsideGain, float32_t fOutsideFreq)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!isfinite(fInsideAngle) || !isfinite(fOutsideAngle) || !isfinite(fOutsideGain) || !isfinite(fOutsideFreq))
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fInsideAngle < kfSourceConeAngleMin || fInsideAngle > kfSourceConeAngleMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fOutsideAngle < kfSourceConeAngleMin || fOutsideAngle > kfSourceConeAngleMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fOutsideGain < kfSourceConeGainMin || fOutsideGain > kfSourceConeGainMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (fOutsideFreq < kfSourceConeFreqMin || fOutsideFreq > kfSourceConeFreqMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (sourceInternal->type() != eSource3d)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    sourceInternal->insideAngle()  = fInsideAngle;
    sourceInternal->outsideAngle() = fOutsideAngle;
    sourceInternal->outsideGain()  = fOutsideGain;
    sourceInternal->outsideFreq()  = fOutsideFreq;

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetPan(SceHwSource hSourceHandle, float32_t fPan)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }
    if (!isfinite(fPan) || fPan < kfSourcePanLeft || fPan > kfSourcePanRight)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (sourceInternal->type() != eSource2d)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    sourceInternal->pan() = fPan;

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetVolume(SceHwSource hSourceHandle, float32_t fVolume)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }
    if (!isfinite(fVolume) || fVolume < kfSourceVolumeMin || fVolume > kfSourceVolumeMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal->volume() = fVolume;

    SCE_HEATWAVE_UNLOCK();

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceSetPitchMultiplier(SceHwSource hSourceHandle, float32_t fPitchMultiplier)
{
    SourceInternal *sourceInternal = NULL;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }
    if (!isfinite(fPitchMultiplier) || fPitchMultiplier < kfSourcePitchMin || fPitchMultiplier > kfSourcePitchMax)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    sourceInternal = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternal->pitch() = fPitchMultiplier;

    SCE_HEATWAVE_UNLOCK();

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSourceGetInfo(SceHwSource hSourceHandle, SceHwSourceInfo* pOutInfo)
{
    SourceInternal *sourceInternalHandle = NULL;

    if (pOutInfo == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    sourceInternalHandle = _sceHeatWaveGetSourceInternal(hSourceHandle);
    if (sourceInternalHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    pOutInfo->nLoopCount = sourceInternalHandle->loopCount();
    pOutInfo->hParentSound = _sceHeatWaveGetSoundHandle(sourceInternalHandle->getParentSound());
    pOutInfo->eType = sourceInternalHandle->type();
    pOutInfo->fGain = sourceInternalHandle->volume();
    pOutInfo->fPitch = sourceInternalHandle->pitch();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}
