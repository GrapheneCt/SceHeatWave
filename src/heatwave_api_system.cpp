/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @history
*   1.1 Add load and unload of MP3 codec libraries.
*
*   1.2 Change sceHeatWaveRender() to also perform SystemInternal::update().
*       Add sceHeatWaveSetCallback().
*       Add sceHeatWaveGetSupportedMimeTypes().
*       Add sceHeatWaveGetInfo().
*
*   1.3 Fix sceHeatWaveGetInfo returning incorrect output type when using Audio3d.
*/

#include "heatwave_api.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_systeminternal.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_file.h"
#include "heatwave_api_synthinterface.h"
#include "heatwave_api_panner.h"
#include "MP3/heatwave_api_decoder_mp3.h"
#include "ATRAC9/heatwave_api_decoder_at9.h"

using namespace sce::HeatWave;

static const uint32_t kuHeatWaveStubVersion = SCE_HEATWAVE_API_VERSION; ///< Constant to store the version in the stub - so that we can check for header\stub mismatch

SceHwError sceHeatWaveGetVersion(uint32_t *uVersion)
{
    if (uVersion == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    *uVersion = kuHeatWaveStubVersion;

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetMemoryHandlers(SceHwMemoryFunctions *pUserOverrides)
{
    if (pUserOverrides == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pAlloc == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pAlignedAlloc == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pFree == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (SystemInternal::getInstance()) /* System has already been setup */
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    MemoryHandler::s_userOverrides.pAlloc           = pUserOverrides->pAlloc;
    MemoryHandler::s_userOverrides.pFree            = pUserOverrides->pFree;
    MemoryHandler::s_userOverrides.pAlignedAlloc    = pUserOverrides->pAlignedAlloc;

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetFileHandlers(SceHwFileFunctions *pUserOverrides)
{
    if (pUserOverrides == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pOpen == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pClose == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pSeek == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pTell == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (pUserOverrides->pRead == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (SystemInternal::getInstance()) /* System has already been setup */
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    FileHandler::s_userOverrides.pOpen  = pUserOverrides->pOpen;
    FileHandler::s_userOverrides.pClose = pUserOverrides->pClose;
    FileHandler::s_userOverrides.pSeek  = pUserOverrides->pSeek;
    FileHandler::s_userOverrides.pTell  = pUserOverrides->pTell;
    FileHandler::s_userOverrides.pRead  = pUserOverrides->pRead;

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveInit(SceHwInitParams *pParams)
{
    PlatformSynth *synth        = NULL;
    SceHwError res              = SCE_HEATWAVE_API_OK;
    const SceHwVector zero      = { 0.0f, 0.0,  0.0f };
    const SceHwVector forward   = { 0.0f, 0.0, -1.0f };
    const SceHwVector up        = { 0.0f, 1.0,  0.0f };

    if (pParams == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (!SystemInternal::getInstance())
    {
        SystemInternal::createInstance();
    }
    if (SystemInternal::getInstance()->isInitialised())
    {
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    if (SystemInternal::getInstance()->getPanner() == NULL)
    {
        sceHeatWaveRelease();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    synth = PlatformSynth::createSynth();
    if (synth == NULL)
    {
        sceHeatWaveRelease();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    res = synth->init(pParams->bLoopBack);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        delete synth;
        sceHeatWaveRelease();
        return res;
    }

    if (!SystemInternal::getInstance()->init(synth))
    {
        sceHeatWaveRelease();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

#if defined __psp2__
    res = DecoderMp3::loadCodecLibraries(pParams->nMaxChannelNumMp3);
#else
    res = DecoderMp3::loadCodecLibraries(0);
#endif
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        delete synth;
        sceHeatWaveRelease();
        return res;
    }

#if defined __psp2__
    res = DecoderAt9::loadCodecLibraries(pParams->nMaxChannelNumAt9);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        delete synth;
        sceHeatWaveRelease();
        return res;
    }
#endif

    /* Default listener to centre */
    sceHeatWaveSetListener(zero, zero, forward, up, 1.0f);

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveRelease(void)
{
    SceHwError nRet = SCE_HEATWAVE_API_OK;
    SystemInternal *systemInternal = NULL;
    PlatformSynth  *synth          = NULL;

    systemInternal = SystemInternal::getInstance();
    if (!systemInternal)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    synth = systemInternal->getSynth();

    systemInternal->release();

    if (synth)
    {
        nRet = synth->release();
        delete synth;
    }

    delete systemInternal;

    DecoderMp3::unloadCodecLibraries();
#if defined __psp2__
    DecoderAt9::unloadCodecLibraries();
#endif

    /* Must be last - all release related code needs to pass through the custom overrides before we clear them */
    FileHandler::s_userOverrides.pOpen  = NULL;
    FileHandler::s_userOverrides.pClose = NULL;
    FileHandler::s_userOverrides.pSeek  = NULL;
    FileHandler::s_userOverrides.pTell  = NULL;
    FileHandler::s_userOverrides.pRead  = NULL;

    MemoryHandler::s_userOverrides.pAlloc           = NULL;
    MemoryHandler::s_userOverrides.pAlignedAlloc    = NULL;
    MemoryHandler::s_userOverrides.pFree            = NULL;

    SfxInternal::setCallback(SCE_HEATWAVE_API_NO_CALLBACK, NULL);

    return nRet;
}

SceHwError sceHeatWaveRender(int16_t* pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal *systemInternal = NULL;
    PlatformSynth  *synth          = NULL;

    if (pBuffer == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    systemInternal->update();

    synth = systemInternal->getSynth();
    if (synth == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = synth->render(pBuffer, uNumSamplesPerChannel, uNumChannels);
    return res;
}

SceHwError sceHeatWaveGetInfo(SceHwInfo *pInfoOut)
{
    SystemInternal *pSystemInternal = NULL;
    PlatformSynth *pSynth = NULL;
    bool bLoopback = false;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (pInfoOut == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    pSystemInternal = SystemInternal::getInstance();
    if (!pSystemInternal)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    pSynth = pSystemInternal->getSynth();
    if (pSynth == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    MemoryHandler::memsetCb(pInfoOut, 0, sizeof(SceHwInfo));

    /* Set the output type for the platform */
#if defined(_WINDOWS)
    pInfoOut->eOutputType = eSysOutput_XAudio2;
#elif defined (__ORBIS__)
    #if defined(SCE_HEATWAVE_API_USE_AUDIO3D)
        pInfoOut->eOutputType = eSysOutput_Audio3d;
    #else
        pInfoOut->eOutputType = eSysOutput_Ngs2;
    #endif /* SCE_HEATWAVE_USE_AUDIO3D */
#elif defined(__PS3__)
    pInfoOut->eOutputType = eSysOutput_Multistream;
#elif defined(__psp2__)
    pInfoOut->eOutputType = eSysOutput_Ngs;
#endif /* _WINDOWS */

    /* If the synth is in loopback mode, the output is actually user render */
    nRet = pSynth->isLoopback(&bLoopback);
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    if (bLoopback)
    {
        pInfoOut->eOutputType = eSysOutput_UserRender;
    }

    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_MAIN]    = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_MAIN);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_AUX]     = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_AUX);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_PAD_1]   = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_PAD_1);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_PAD_2]   = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_PAD_2);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_PAD_3]   = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_PAD_3);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_PAD_4]   = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_PAD_4);
    pInfoOut->bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_AUDIO3D] = pSynth->isPortSupported(SCE_HEATWAVE_API_OUTPUT_AUDIO3D);

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveOpenUserOutput(int32_t nPadNumber, int32_t nUserServiceID)
{
    SystemInternal *pSystemInternal = NULL;
    PlatformSynth *pSynth = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (nPadNumber < SCE_HEATWAVE_API_OUTPUT_PAD_1 || nPadNumber > SCE_HEATWAVE_API_OUTPUT_PAD_4)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    pSystemInternal = SystemInternal::getInstance();
    if (pSystemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    pSynth = pSystemInternal->getSynth();
    if (pSynth == NULL)
    {
          return SCE_HEATWAVE_API_ERROR;
    }

    if (!pSynth->isPortSupported(nPadNumber))
    {
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    nRet = pSynth->openUserPort(nPadNumber, nUserServiceID);
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveCloseUserOutput(int32_t nPadNumber)
{
    SystemInternal *pSystemInternal = NULL;
    PlatformSynth *pSynth = NULL;
    SceHwError nRet = SCE_HEATWAVE_API_OK;

    if (nPadNumber < SCE_HEATWAVE_API_OUTPUT_PAD_1 || nPadNumber > SCE_HEATWAVE_API_OUTPUT_PAD_4)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    pSystemInternal = SystemInternal::getInstance();
    if (pSystemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    pSynth = pSystemInternal->getSynth();
    if (pSynth == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (!pSynth->isPortSupported(nPadNumber))
    {
        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
    }

    nRet = pSynth->closeUserPort(nPadNumber);
    if (SCE_HEATWAVE_API_FAILED(nRet))
    {
        return nRet;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetListener(SceHwVector vPosition, SceHwVector vVelocity, SceHwVector vForward, SceHwVector vUp, float32_t fGain)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->setListenerPosition(vPosition);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    res = panner->setListenerVelocity(vVelocity);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    res = panner->setListenerOrientation(vForward, vUp);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    res = panner->setListenerGain(fGain);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    /* Flush the updated listener settings through */
    SCE_HEATWAVE_LOCK();
    res = system->updateParameters();
    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetCallback(SceHwSystemCallbackFunction pCallbackFunc, void *pUserData /*= NULL*/)
{
    SystemInternal  *systemInternal = NULL;
    SceHwError ret = SCE_HEATWAVE_API_OK;

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (pUserData != NULL && pCallbackFunc == SCE_HEATWAVE_API_NO_CALLBACK)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();
    ret = systemInternal->setCallback(pCallbackFunc, pUserData);
    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(ret))
    {
        return ret;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetSfxCallback(SceHwSfxCallbackFunction pCallbackFunc, void *pUserData /*= NULL*/)
{
    SceHwError ret = SCE_HEATWAVE_API_OK;

    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    if (pUserData != NULL && pCallbackFunc == SCE_HEATWAVE_API_NO_CALLBACK)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    SCE_HEATWAVE_LOCK();

    ret = SfxInternal::setCallback(pCallbackFunc, pUserData);

    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(ret))
    {
        return ret;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveCreateSound(SceHwSound *pOutSoundHandle)
{
    SystemInternal  *systemInternal = NULL;
    SoundInternal   *newSoundHandle = NULL;

    if (pOutSoundHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    if (!systemInternal->createSound(&newSoundHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS;
    }

    *pOutSoundHandle = _sceHeatWaveGetSoundHandle(newSoundHandle);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveReleaseSound(SceHwSound hSoundHandle)
{
    SystemInternal  *systemInternal = NULL;
    SoundInternal   *soundHandle    = NULL;

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    soundHandle = _sceHeatWaveGetSoundInternal(hSoundHandle);
    if (soundHandle == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_HANDLE;
    }

    if (!systemInternal->releaseSound(soundHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_OK;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @brief Retrieve the nth valid sound handle in the system
* @param nIndex The nth valid sound to return (not an array index)
* @note <c><i>nIndex</i></c> is the nth occupied sound slot. Internally the array <i>could</i> have gaps.
*/
SceHwError sceHeatWaveGetSound(int32_t nIndex, SceHwSound *pOutSoundHandle)
{
    uint32_t count = 0;
    SoundInternal   *soundInternal  = NULL;
    SystemInternal  *systemInternal = NULL;

    if (pOutSoundHandle == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    if (nIndex < 0 || nIndex >= systemInternal->getSoundCount())
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    while (nIndex >= 0 && count < _sceHeatWaveGetMaxSoundCount())
    {
        soundInternal = systemInternal->getSound(count);
        if (soundInternal != NULL)
        {
            --nIndex; /* The slot was occupied */
        }
        count++;
    }

    if (soundInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    *pOutSoundHandle = _sceHeatWaveGetSoundHandle(soundInternal);

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

/**
* @brief The number of occupied sound slots
*/
SceHwError sceHeatWaveGetSoundCount(int32_t *pOutCount)
{
    SystemInternal *systemInternal = NULL;

    if (pOutCount == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (SystemInternal::getInstance() == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_LOCK();

    systemInternal = SystemInternal::getInstance();
    if (systemInternal == NULL)
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    *pOutCount = systemInternal->getSoundCount();

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveReleaseSfx(SceHwSfx hSfxHandle)
{
    SfxInternal     *sfxInternalHandle = NULL;
    SoundInternal   *soundInternalHandle = NULL;

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

    soundInternalHandle = sfxInternalHandle->getParentSound();
    if (soundInternalHandle == NULL) /* hSfxHandle does not belong to a sound, bad! */
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (!soundInternalHandle->releaseSfx(sfxInternalHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveReleaseSource(SceHwSource hSourceHandle)
{
    SourceInternal  *sourceInternalHandle = NULL;
    SoundInternal   *soundInternalHandle = NULL;

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

    soundInternalHandle = sourceInternalHandle->getParentSound();
    if (!soundInternalHandle) /* hSourceHandle does not belong to a sound, bad! */
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    if (!soundInternalHandle->releaseSource(sourceInternalHandle))
    {
        SCE_HEATWAVE_UNLOCK();
        return SCE_HEATWAVE_API_ERROR;
    }

    SCE_HEATWAVE_UNLOCK();
    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveGetSupportedMimeTypes(const char **ppMimeList[], uint32_t *pNumEntries)
{
    static const char* mimelist[] = {
        "audio/wav",
        "audio/mpeg3",
#ifdef __psp2__
        "audio/adpcm",
        "audio/ATRAC9"
#endif
    };

    if (ppMimeList == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }
    if (pNumEntries == NULL)
    {
        return SCE_HEATWAVE_API_ERROR_PARAM;
    }

    *ppMimeList = mimelist;
    *pNumEntries = sizeof(mimelist) / sizeof(mimelist[0]);

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetSpeedOfSound(float32_t fSpeedOfSound)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->setSpeedOfSound(fSpeedOfSound);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    /* Flush the updated listener settings through */
    SCE_HEATWAVE_LOCK();
    res = system->updateParameters();
    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveGetSpeedOfSound(float32_t *pfSpeedOfSound)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->getSpeedOfSound(pfSpeedOfSound);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetDopplerFactor(float32_t fDopplerFactor)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->setDopplerFactor(fDopplerFactor);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    /* Flush the updated listener settings through */
    SCE_HEATWAVE_LOCK();
    res = system->updateParameters();
    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveGetDopplerFactor(float32_t *pfDopplerFactor)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->getDopplerFactor(pfDopplerFactor);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveSetDistanceModel(int32_t nModel)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->setDistanceModel(nModel);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    /* Flush the updated listener settings through */
    SCE_HEATWAVE_LOCK();
    res = system->updateParameters();
    SCE_HEATWAVE_UNLOCK();

    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}

SceHwError sceHeatWaveGetDistanceModel(int32_t *pnModel)
{
    SceHwError res = SCE_HEATWAVE_API_OK;
    SystemInternal  *system = NULL;
    Panner          *panner = NULL;


    system = SystemInternal::getInstance();
    if (system == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    panner = system->getPanner();
    if (panner == NULL)
    {
        return SCE_HEATWAVE_API_ERROR;
    }

    res = panner->getDistanceModel(pnModel);
    if (SCE_HEATWAVE_API_FAILED(res))
    {
        return res;
    }

    return SCE_HEATWAVE_API_OK;
}
