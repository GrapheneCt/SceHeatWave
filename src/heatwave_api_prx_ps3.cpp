/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file     heatwave_api_prx_ps3.cpp
*
* @brief    Export definitions for the PS3 PRX build of the library
*
* @history
*   1.2 File added
*/

#include <cellstatus.h>
#include <sys/prx.h>

SYS_MODULE_INFO(libSceHeatWave, 0, 1, 44); ///< Name and version

/* Name of the stub (append D in Debug) */
#ifdef _DEBUG
    SYS_LIB_DECLARE_WITH_STUB(libSceHeatWave, SYS_LIB_AUTO_EXPORT | SYS_LIB_WEAK_IMPORT, libSceHeatWaveD_stub);
#else /* !_DEBUG*/
    SYS_LIB_DECLARE_WITH_STUB(libSceHeatWave, SYS_LIB_AUTO_EXPORT | SYS_LIB_WEAK_IMPORT, libSceHeatWave_stub);
#endif

SYS_LIB_EXPORT(sceHeatWaveGetVersion, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveInit, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveRelease, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveRender, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetInfo, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveOpenUserOutput, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveCloseUserOutput, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetListener, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetCallback, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetSfxCallback, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveCreateSound, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveReleaseSound, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveReleaseSfx, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveReleaseSource, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundCreateSfx, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundCreateSource, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceLoadFile, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceLoadMemory, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceLoadRuntimeStream, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxPlay, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxStop, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxPause, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxResume, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSetVolume, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxGetVolume, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSetPitchMultiplier, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxGetPitchMultiplier, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSetPosition, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSetVelocity, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSetForward, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxSeek, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetMemoryHandlers, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetFileHandlers, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetSound, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetSoundCount, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundGetSfx, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundGetSfxCount, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundGetSource, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundGetSourceCount, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundGetInfo, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSoundSetLoopCount, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetType, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetLoopCount, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetRolloffDistances, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetDistanceFactor, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetCone, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetPan, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetVolume, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceSetPitchMultiplier, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSourceGetInfo, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxGetState, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSfxGetInfo, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetSupportedMimeTypes, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetSpeedOfSound, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetSpeedOfSound, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetDopplerFactor, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetDopplerFactor, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveSetDistanceModel, libSceHeatWave);
SYS_LIB_EXPORT(sceHeatWaveGetDistanceModel, libSceHeatWave);

