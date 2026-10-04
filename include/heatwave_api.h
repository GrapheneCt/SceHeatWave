/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api.h
*
* @brief Main HeatWave API header file
*
* @history
*   1.2     Add functionality to allow playback of SceHwSfx as Audio3d objects.
*           Add shared library build.
*           Add support for callbacks.
*           Add functionality to query supported audio formats as MIME types.
*           Add volume and pitch to SceHwSource.
*           Add constant for invalid handle.
*           Add functionality to query global information.
*   1.3     Add looping to SceHwSound (causes SceHwSfx to loop during playback).
*           Add support for runtime generation of audio data.
*   1.4     Change the SceHwSound, SceHwSfx & SceHwSource handle format to allow increased instances of each.
*           Change value of kuSceHwInvalidHandle.
*           Add support for output to multiple destinations (Main, Pad, Aux).
*           Add sceHeatWaveOpenUserOutput() & sceHeatWaveCloseUserOutput().
*   1.41    Fix sceHeatWaveOpenUserOutput() returning SCE_HEATWAVE_API_OK when passed an invalid nUserServiceID.
*           Change sceHeatWaveCloseUserOutput() to return SCE_HEATWAVE_API_ERROR_PARAM if nOutputId was not previously opened by sceHeatWaveOpenUserOutput().
*   1.42    Remove link to unused Sulpha library.
*   1.43    Fix SceAudio3dOpenParameters to be initialised with default values.
*   1.44    Add support to query the duration of a SceHwSound.
*           Add support to query the elapsed time of a playing SceHwSfx.
*           Add seek support to SceHwSfx.
*           Fix looping behaviour when SceHwSource of a looping SceHwSound have different loop counts. Note - this changes behaviour of looping sounds.
*   1.5     Fix memory leak in MP3 decoder on Windows.
*           Implement true 2D audio for eSource2d.
*           Add distance model support.
*           Add doppler factor support.
*           Add OGG codec support.
*           Add OPUS codec support.
*           Add PAF build support (PSVita only).
*           
*/

#ifndef _SCE_HEATWAVE_API_H_
#define _SCE_HEATWAVE_API_H_

#include "heatwave_api_types.h"
#include <limits>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif /* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

/**
* @brief The version number in hex format (for example 0x10132AD7).
*
* The format of the version number is MmmXXXXX
* where M is the major version, mm is the minor version and XXXXX is the revision.
*/
#define SCE_HEATWAVE_API_VERSION 0x10500000

/**
* @brief Used to mark HeatWave API functions as deprecated.
*
* Use of deprecated functions should be avoided as they will be removed in later versions of the library.
*/
#if defined(__GNUC__) || defined(__SNC__)
    #define SCE_HEATWAVE_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
    #define SCE_HEATWAVE_DEPRECATED(func) __declspec(deprecated) func
#endif

/**
* @brief Marker for HeatWave API functions.
*
* Used to specify the API functions as dynamic linkage.
*/
#ifdef SCE_HEATWAVE_DYNAMIC
    #ifdef SCE_HEATWAVE_EXPORT
        #if defined __ORBIS__ || defined _WINDOWS || defined __psp2__
            #define SCE_HEATWAVE_FUNC  __declspec(dllexport)
        #endif
    #else
        #if defined __ORBIS__ || defined _WINDOWS || defined __psp2__
                #define SCE_HEATWAVE_FUNC  __declspec(dllimport)
        #endif
    #endif
#endif /* SCE_HEATWAVE_DYNAMIC */

#ifndef SCE_HEATWAVE_FUNC
    #define SCE_HEATWAVE_FUNC
#endif

/**
* @brief Constant for a handle that is not valid.
*
* @see SceHwSound, SceHwSource, SceHwSfx
*/
const SceHwHandle kuSceHwInvalidHandle = ULLONG_MAX;

/**
* @brief Sets a <c>SceHwSource</c> to loop until sceHeatWaveSfxStop() is called.
*
* @see sceHeatWaveSourceSetLoopCount(), sceHeatWaveSfxStop()
*/
const int32_t knLoopInfinite = -1;

/**
* @brief Sets the minimum <c>SceHwSfx</c> volume multiplier.
*
* @see sceHeatWaveSfxSetVolume()
*/
const float32_t kfSfxVolumeMin = 0.0f;

/**
* @brief Sets the maximum <c>SceHwSfx</c> volume multiplier.
*
* @see sceHeatWaveSfxSetVolume()
*/
const float32_t kfSfxVolumeMax = 1.0f;

/**
* @brief Sets the minimum <c>SceHwSfx</c> pitch multiplier.
*
* @see sceHeatWaveSfxSetPitchMultiplier()
*/
const float32_t kfSfxPitchMin = 0.0f;

/**
* @brief Sets the maximum <c>SceHwSfx</c> pitch multiplier.
*
* @see sceHeatWaveSfxSetPitchMultiplier()
*/
const float32_t kfSfxPitchMax = 2.0f;

/**
* @brief Sets the minimum angle an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeAngleMin = 0.0f;

/**
* @brief Sets the maximum angle an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeAngleMax = 360.0f;

/**
* @brief Sets the minimum gain an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeGainMin = 0.0f;

/**
* @brief Sets the maximum gain an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeGainMax = 1.0f;

/**
* @brief Sets the minimum frequency an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeFreqMin = 0.0f;

/**
* @brief Sets the maximum frequency an emitter cone can have.
*
* @see sceHeatWaveSourceSetCone()
*/
const float32_t kfSourceConeFreqMax = 1.0f;

/**
* @brief Sets the maximum left pan a <c>SceHwSource</c> can have.
*
* @see sceHeatWaveSourceSetPan()
*/
const float32_t kfSourcePanLeft = -1.0f;

/**
* @brief Sets the maximum right pan a <c>SceHwSource</c> can have.
*
* @see sceHeatWaveSourceSetPan()
*/
const float32_t kfSourcePanRight = 1.0f;

/**
* @brief Sets the minimum <c>SceHwSource</c> volume multiplier.
*
* @see sceHeatWaveSourceSetVolume()
*/
const float32_t kfSourceVolumeMin = 0.0f;

/**
* @brief Sets the maximum <c>SceHwSource</c> volume multiplier.
*
* @see sceHeatWaveSourceSetVolume()
*/
const float32_t kfSourceVolumeMax = 1.0f;

/**
* @brief Sets the minimum <c>SceHwSource</c> pitch multiplier.
*
* @see sceHeatWaveSourceSetPitchMultiplier()
*/
const float32_t kfSourcePitchMin = 0.0f;

/**
* @brief Sets the maximum <c>SceHwSource</c> pitch multiplier.
*
* @see sceHeatWaveSourceSetPitchMultiplier()
*/
const float32_t kfSourcePitchMax = 2.0f;

/**
* @brief The highest priority an Audio3d <c>SceHwSfx</c> can have.
*
* @see sceHeatWaveSfxPlay()
*/
const int32_t knSfxAudio3dPriorityMax = 0;

/**
* @brief The lowest priority a <c>SceHwSfx</c> Audio3d object can have.
*
* @see sceHeatWaveSfxPlay()
*/
const int32_t knSfxAudio3dPriorityMin = INT_MAX;

/**
* @brief The duration is infinite.
*
* In the case of infinite looping the duration will also be infinite.
*
* @see sceHeatWaveSoundGetInfo()
*/
const int32_t knDurationInfinite = -1;

/**
* @brief The duration is unknown.
*
* In some instances it is not possible to calculate a duration.
*
* @see sceHeatWaveSoundGetInfo()
*/
const int32_t knDurationUnknown = -2;

/**
* @brief The current playback time is unknown.
*
* On some platforms the elapsed playback time is not supported.
*
* @see sceHeatWaveSfxGetInfo()
*/
const int32_t knCurrentTimeUnkown = -1;

/**
* @brief Do not execute a user callback.
*
* @see sceHeatWaveSetCallback(), sceHeatWaveSetSfxCallback()
*/
#define SCE_HEATWAVE_API_NO_CALLBACK (NULL)

/**
* @brief Gets the version identifier of the HeatWave library.
*
* @param[out]    uVersion   Receives the version identifier.
*
* @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see <c>SCE_HEATWAVE_API_VERSION</c>
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetVersion(uint32_t *uVersion);

/**
* @brief    Initializes the HeatWave library.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @param[in]    pParams     The initialization parameters.
*
* @note     This function <b>must</b> be called before any other HeatWave function except for sceHeatWaveSetMemoryHandlers() and sceHeatWaveSetFileHandlers().
*
* @see      sceHeatWaveRelease(), sceHeatWaveSetMemoryHandlers(), sceHeatWaveSetFileHandlers()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveInit(SceHwInitParams *pParams);

/**
* @brief    Releases all resources used by the HeatWave library.
*
*           This function releases all sounds, sound sources and sound effects that have not been released at the point it was called.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveInit()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveRelease(void);

/**
* @brief        Render a block of audio data into a buffer.
*
*               This function will only work when the Heatwave library is in loopback mode and needs to be called at a rate suitable for the destination the audio data is being rendered to.
*               It can be used, for example, when there is a requirement to write the audio data to a file.
*
* @param[in]    pBuffer                 The memory area to render the audio data to.
* @param[in]    uNumSamplesPerChannel   The number of audio samples to render for each channel.
* @param[in]    uNumChannels            The number of channels.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes. A value of <c>SCE_HEATWAVE_API_ERROR_OPERATION</c> is returned if HeatWave is not in loopback mode.
*
* @see          sceHeatWaveInit()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveRender(int16_t* pBuffer, uint32_t uNumSamplesPerChannel, uint32_t uNumChannels);

/**
* @brief        Gets global information about HeatWave.
*
* @param[out]   pInfoOut    Receives the information about HeatWave.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          SceHwInfo
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetInfo(SceHwInfo *pInfoOut);

/**
* @brief Open a user-specific user audio output.
*
* Open the audio output destination for the specified user handle.
*
* @param nOutputId      SCE_HEATWAVE_API_OUTPUT_PAD_XXX.
* @param nUserServiceID The platform handle representing the user service ID.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveOpenUserOutput(int32_t nOutputId, int32_t nUserServiceID);

/**
* @brief Close a user-specific audio output.
*
* Close a previously opened user-specific audio output.
*
* @param nOutputId SCE_HEATWAVE_API_OUTPUT_PAD_XXX.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveCloseUserOutput(int32_t nOutputId);

/**
* @brief        Sets the listener in 3D space.
*
*               Typically the listener is set to the same position, velocity and orientation as the game camera and can be thought of as a
*               "virtual microphone". The HeatWave library only supports one listener, which is always available once the library has been initialized.
*               This function is used to update the listener's position, velocity and rotation orientation in the game as required.
*
* @param[in]    vPosition   The 3D world position of the listener.
* @param[in]    vVelocity   The velocity of the listener, which is used for applying Doppler shift. Because this argument is not used to move the listener,
*                           it can be set to a greater or lesser value than the listener is actually moving at. This can be used to create a more or less
*                           extreme Doppler shift as required. Setting this argument to 0 means no Doppler shift will be applied unless a sound effect
*                           has a velocity of greater than 0.
* @param[in]    vForward    The direction the listener is facing.
* @param[in]    vUp         The normal to the direction vector, which is used to determine what is left and what is right for the listener.
* @param[in]    fGain       Gain of the listener object
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note         A right hand coordinate system is used.
* @note         The value passed to the <c><i>vVelocity</i></c> parameter <b>must</b> be less than <c>sceHeatWaveGetSpeedOfSound</c>.
*
* @see          SceHwVector
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetListener(SceHwVector vPosition, SceHwVector vVelocity, SceHwVector vForward, SceHwVector vUp, float32_t fGain);

/**
* @brief        Set the user callback for HeatWave.
*
*               The callback reports changes in the internal state of HeatWave.  <c>SceHwSystemCallbackType</c> specifies
*               the conditions that will cause this callback to be executed.
*
* @param[in]    pCallbackFunc   The user function to execute.
* @param[in]    pUserData       User specific data passed to the callback function (optional).
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note         A value of <c>SCE_HEATWAVE_API_NO_CALLBACK</c> for <c>pCallbackFunc</c> indicates the callback should be disabled.
* @note         <c>pUserData</c> is optional, a value of <c>NULL</c> indicates no user data should be passed when executing <c>pCallbackFunc</c>.
*
* @see          SceHwSystemCallbackType, SceHwSystemCallbackFunction
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetCallback(SceHwSystemCallbackFunction pCallbackFunc, void *pUserData = NULL);

/**
* @brief        Set the user callback for <c>SceHwSfx</c>.
*
*               The callback reports changes in the internal state of <c>SceHwSfx</c>.  <c>SceHwSfxCallbackInfo</c> specifies the
*               conditions that will cause the callback to be executed.  The callback function is common to all <c>SceHwSfx</c>.
*
* @param[in]    pCallbackFunc   The user function to execute
* @param[in]    pUserData       User specific data passed to the callback function (optional)
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note         A value of <c>SCE_HEATWAVE_API_NO_CALLBACK</c> for <c>pCallbackFunc</c> indicates the callback should be disabled.
* @note         <c>pUserData</c> is optional, with a value of <c>NULL</c> indicating that no user data should be passed when executing <c>pCallbackFunc</c>.
*
* @see          SceHwSfxCallbackType, SceHwSfxCallbackInfo
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetSfxCallback(SceHwSfxCallbackFunction pCallbackFunc, void *pUserData = NULL);

/**
* @brief        Creates a new sound.
*
*               At least one sound must be created before any sound effects can be created. When a sound is no longer needed,
*               the resources it uses should be released by calling sceHeatWaveReleaseSound().
*
* @param[out]   pOutSoundHandle    Receives a handle to the newly created sound if the operation was successful.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          sceHeatWaveReleaseSound(), sceHeatWaveGetSound(), sceHeatWaveGetSoundCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveCreateSound(SceHwSound *pOutSoundHandle);

/**
* @brief        Releases an existing sound.
*
*               Calling this function frees up the resources used by the sound.
*               This includes all sound effects referencing a sound and all sound sources belonging to a sound.
*               Internally, this will be done with calls to sceHeatWaveReleaseSfx() and sceHeatWaveReleaseSource().
*
* @param[in]    hSoundHandle    The sound to release.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          sceHeatWaveCreateSound(), sceHeatWaveGetSound(), sceHeatWaveGetSoundCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveReleaseSound(SceHwSound hSoundHandle);

/**
* @brief        Releases an existing sound effect.
*
*               Calling this function frees up the resources used by the sound effect and stops it playing immediately.
*
* @param[in]    hSfxHandle     The sound effect to be released.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          sceHeatWaveSoundCreateSfx(), sceHeatWaveSoundGetSfx(), sceHeatWaveSoundGetSfxCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveReleaseSfx(SceHwSfx hSfxHandle);

/**
* @brief        Releases an existing sound source.
*
*               Calling this function frees up the resources used by the sound source. A sound source that has been
*               released will not be played anymore when the sound it belonged to is played by a sound effect.
*
* @param[in]    hSourceHandle  The audio source to be released.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          sceHeatWaveSoundCreateSource(), sceHeatWaveSoundGetSource(), sceHeatWaveSoundGetSourceCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveReleaseSource(SceHwSource hSourceHandle);

/**
* @brief        Creates a sound effect that references the specified sound.
*
* @param[in]    hSoundHandle    The sound to get a sound effects handle for.
* @param[out]   pOutSfxHandle   Receives the newly created sound effect handle if the operation was successful.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveReleaseSfx(), sceHeatWaveSoundGetSfx(), sceHeatWaveSoundGetSfxCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundCreateSfx(SceHwSound hSoundHandle, SceHwSfx *pOutSfxHandle);

/**
* @brief        Creates a new sound source for a sound.
*
*               The sound source created by this function needs to be populated with data from
*               an audio file before it will be possible to hear anything. Either sceHeatWaveSourceLoadFile() or
*               sceHeatWaveSourceLoadMemory() should be used to do this. The sound source should be specified as 2D or 3D
*               by calling sceHeatWaveSourceSetType(). It is also possible to set the 3D cone
*               and roll-off distance (only for 3D sounds), the pan position (only for 2D sounds),
*               and the loop count for the sound source.
*
* @param[in]    hSoundHandle       The sound to add a new audio source to.
* @param[out]   pOutSourceHandle   Receives a handle to the newly created sound source if the operation was successful.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see          sceHeatWaveReleaseSource(), sceHeatWaveSoundGetSource(), sceHeatWaveSoundGetSourceCount(), sceHeatWaveSourceLoadFile(), sceHeatWaveSourceLoadMemory(), sceHeatWaveSourceSetType()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundCreateSource(SceHwSound hSoundHandle, SceHwSource *pOutSourceHandle);

/**
* @brief        Sets the audio file for a sound source to use.
*
*               Once this function has been called, the audio from the supplied file will be used when
*               the parent sound is required to play.
*
* @param[in]    hSourceHandle       The audio source that will own the audio file.
* @param[in]    strFilePath         The path to the audio file.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note         MP3 and WAV file format (16-bit PCM only) are supported.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceLoadFile(SceHwSource hSourceHandle, const char *strFilePath);

/**
* @brief        Sets a sound source to use some audio data loaded in memory.
*
*               Once this function has been called, the supplied audio data will be used when
*               the parent sound is required to play. The audio data should
*               come from a previously loaded audio file.
*
* @param[in] hSourceHandle  The audio source that will own the audio data.
* @param[in] pMemoryFile    The location in memory of the audio data.
*
* @return                   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note                     MP3 and WAV file format (16-bit PCM only) is supported.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceLoadMemory(SceHwSource hSourceHandle, SceHwMemoryFileInfo *pMemoryFile);

/**
* @brief    Set the callback used to generate audio data for instances of the <c>SceHwSource</c>.
*
*           Once this function has been called the supplied callback will be used to generate audio
*           data for each instance of the <c>SceHwSource</c>.
*
* @param[in] hSourceHandle  The audio source that will own the audio data.
* @param[in] pParam         Information about the stream.
* @param[in] pFunc          The callback function used to generate audio data.
* @param[in] pUserData      User specified data passed to the callback function (optional).
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     16 bit PCM only is supported.
*
* @see      SceHwSourceRuntimeStreamCallbackFunction
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceLoadRuntimeStream(SceHwSource hSourceHandle, SceHwSourceRuntimeStreamParam* pParam, SceHwSourceRuntimeStreamCallbackFunction pFunc, void *pUserData = NULL);

/**
* @brief        Plays a sound effect.
*
*               This function will play all the sound sources that belong to the sound that
*               this sound effect references.
*
* @param[in]    hSfxHandle      The sound effect to play.
* @param[in]    pParam          Settings to control playback.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes
*
* @see      sceHeatWaveSfxStop(), sceHeatWaveSfxPause(), sceHeatWaveSfxResume(), sceHeatWaveSfxGetState()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxPlay(SceHwSfx hSfxHandle, SceHwSfxPlayParam *pParam = NULL);

/**
* @brief        Stops a sound effect from playing.
*
*               When this function is called, all sound sources being played for this particular sound effect will stop.
*
* @param[in]    hSfxHandle      The sound effect to stop.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxPlay(), sceHeatWaveSfxPause(), sceHeatWaveSfxResume(), sceHeatWaveSfxGetState()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxStop(SceHwSfx hSfxHandle);

/**
* @brief        Pauses a sound effect.
*
*               When this function is called, all sound sources being played for this particular sound effect
*               will stop playing, but the point at which they stopped and, where applicable,
*               the number of loops they have completed will be remembered. If sceHeatWaveSfxResume() is called,
*               play will continue from this point.
*
* @param[in]    hSfxHandle  The sound effect to pause.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxPlay(), sceHeatWaveSfxStop(), sceHeatWaveSfxResume(), sceHeatWaveSfxGetState()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxPause(SceHwSfx hSfxHandle);

/**
* @brief    Resumes playback of a paused sound effect.
*
* @param[in]    hSfxHandle  The sound effect to resume playing.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxPlay(), sceHeatWaveSfxStop(), sceHeatWaveSfxPause(), sceHeatWaveSfxGetState()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxResume(SceHwSfx hSfxHandle);

/**
* @brief    Sets the playback volume (gain) of a sound effect.
*
* @param[in]   hSfxHandle     The sound effect to set the volume for.
* @param[in]   fVolume        The new volume for the sound effect. This must be a value between <c>kfSfxVolumeMin</c> and <c>kfSfxVolumeMax</c>.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxGetVolume()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSetVolume(SceHwSfx hSfxHandle, float32_t fVolume);

/**
* @brief    Gets the playback volume (gain) of a sound effect.
*
* @param[in]    hSfxHandle     The sound effect to get the volume for.
* @param[out]   pOutVolume     Receives the volume at which the sound effect is playing if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxSetVolume()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxGetVolume(SceHwSfx hSfxHandle, float32_t *pOutVolume);

/**
* @brief    Sets the playback pitch for a sound effect.
*
* @param[in]    hSfxHandle          The sound effect to set the pitch multiplier for.
* @param[in]    fPitchMultiplier    The new pitch multiplier for the sound effect. This must be a value between <c>kfSfxPitchMin</c> and <c>kfSfxPitchMax</c>.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxGetPitchMultiplier()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSetPitchMultiplier(SceHwSfx hSfxHandle, float32_t fPitchMultiplier);

/**
* @brief    Gets the playback pitch of a sound effect.
*
* @param[in]    hSfxHandle              The sound effect to get the pitch multiplier for.
* @param[out]   pOutPitchMultiplier     Receives the pitch multiplier for the sound effect if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxSetPitchMultiplier()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxGetPitchMultiplier(SceHwSfx hSfxHandle, float32_t *pOutPitchMultiplier);

/**
* @brief    Sets the 3D world position for a sound effect.
*
*           This function is used to set the x, y and z coordinates for a sound effect in 3D space.
*           The sound cones for the sound sources that will be played
*           as part of this sound effect will emanate from this point. Sound cones
*           for individual sound sources are set by calling sceHeatWaveSourceSetCone().
*
* @param[in]    hSfxHandle     The sound effect to set the 3D world position to for.
* @param[in]    vPosition      The new 3D world coordinates for the sound effect.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     A right hand coordinate system is used for this operation.
*
* @note     This function is only relevant when a sound source being played for this sound effect
*           has been defined as 3D.
*
* @see      sceHeatWaveSfxSetVelocity(), sceHeatWaveSfxSetForward(), SceHwVector, sceHeatWaveSourceSetCone()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSetPosition(SceHwSfx hSfxHandle, SceHwVector vPosition);

/**
* @brief    Sets the velocity of a sound effect.
*
*           This function sets the velocity of the sound effect, which is used for applying Doppler shift. Because this value is not used to move the sound effect,
*           it can be set to a greater or lesser value than the sound effect is actually moving at. This can be used to create a more or less
*           extreme Doppler shift as required. Setting the velocity of the sound effect to 0 means no Doppler shift will be applied for it unless
*           the listener has a velocity greater than 0.
*
* @param[in]    hSfxHandle      The sound effect to update.
* @param[in]    vVelocity       The new velocity for the sound effect.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     A right hand coordinate system is used for this operation.
*
* @note     The value passed to the <c><i>vVelocity</i></c> parameter <b>must</b> be less than <c>sceHeatWaveGetSpeedOfSound</c>.
*
* @note     This function is only relevant when a sound source being played for this sound effect
*           has been defined as 3D.
*
* @see      sceHeatWaveSfxSetPosition(), sceHeatWaveSfxSetForward(), SceHwVector
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSetVelocity(SceHwSfx hSfxHandle, SceHwVector vVelocity);

/**
* @brief    Sets the direction of a sound effect in 3D space.
*
*           This function is used to set the sound cone direction
*           for the sound sources that will be played as part of this sound effect. Sound cones
*           for individual sound sources are set by calling sceHeatWaveSourceSetCone().
*
* @param[in]    hSfxHandle      The sound effect to update.
* @param[in]    vForward        The new direction for the sound effect.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     A right hand coordinate system is used for this operation.
*
* @note     This function is only relevant when a sound source being played for this sound effect
*           has been defined as 3D.
*
* @see      sceHeatWaveSfxSetPosition(), sceHeatWaveSfxSetVelocity(), SceHwVector, sceHeatWaveSourceSetCone()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSetForward(SceHwSfx hSfxHandle, SceHwVector vForward);

/**
* @brief    Set the current playback time.
*
* @param[in] hSfxHandle The sound effect to update.
* @param[in] nTimeMs    The new playback time in milli-seconds, offset from 0.
*
* @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxSeek(SceHwSfx hSfxHandle, int32_t nTimeMs);

/**
* @brief    Overrides the default memory allocation and deallocation functions that HeatWave uses.
*
* @param[in]    pUserOverrides  The function pointers to the user defined functions, which will override the defaults.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     This function <b>must</b> be called before sceHeatWaveInit().
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetMemoryHandlers(SceHwMemoryFunctions *pUserOverrides);

/**
* @brief    Overrides the default file operation functions that HeatWave uses.
*
* @param[in]    pUserOverrides  The function pointers to the user defined functions, which will override the defaults.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     This function <b>must</b> be called before sceHeatWaveInit().
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetFileHandlers(SceHwFileFunctions *pUserOverrides);

/**
* @brief    Gets an existing sound by index.
*
*           The sounds currently existing in HeatWave are held in a zero-based array.
*
* @param[in]    nIndex             The index of the sound to retrieve.
* @param[out]   pOutSoundHandle    Receives the sound.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveGetSoundCount(), sceHeatWaveCreateSound(), sceHeatWaveReleaseSound()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetSound(int32_t nIndex, SceHwSound *pOutSoundHandle);

/**
*   @brief      Gets the number of sounds that exist.
*
*               This function can be used in conjunction with sceHeatWaveGetSound() to loop through
*               all the sounds currently existing in HeatWave.
*
* @param[out]    pOutCount  Receives the number of sounds.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveGetSound(), sceHeatWaveCreateSound(), sceHeatWaveReleaseSound()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetSoundCount(int32_t *pOutCount);

/**
* @brief    Gets an existing sound effect, which references a particular sound, by index.
*
*           The sound effects referencing a sound are held in a zero-based array.
*
* @param[in]    hSoundHandle   The sound that is referenced by the sound effect.
* @param[in]    nIndex         The index of the sound effect to retrieve.
* @param[out]   pOutSfxHandle  Receives the sound effect.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundGetSfxCount(), sceHeatWaveSoundCreateSfx(), sceHeatWaveReleaseSfx()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundGetSfx(SceHwSound hSoundHandle, int32_t nIndex, SceHwSfx *pOutSfxHandle);

/**
* @brief    Gets the number of sound effects that reference a particular sound.
*
*           This function can be used in conjunction with sceHeatWaveSoundGetSfx() to loop through
*           all the sound effects that reference a sound.
*
* @param[in]    hSoundHandle    The sound to query.
* @param[out]   pOutSfxCount    Receives the number of sound effects that reference the sound if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundGetSfx(), sceHeatWaveSoundCreateSfx(), sceHeatWaveReleaseSfx()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundGetSfxCount(SceHwSound hSoundHandle, int32_t *pOutSfxCount);

/**
* @brief    Gets an existing sound source belonging to a sound by index.
*
*           The sound sources belonging a sound are held in a zero-based array.
*
* @param[in]    hSoundHandle       The sound to get the sound source from.
* @param[in]    nIndex             The index of the sound source to retrieve.
* @param[out]   pOutSourceHandle   Receives the sound source if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundGetSourceCount(), sceHeatWaveSoundCreateSource(), sceHeatWaveReleaseSource()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundGetSource(SceHwSound hSoundHandle, int32_t nIndex, SceHwSource *pOutSourceHandle);

/**
* @brief    Gets the number of sound sources that belong to a particular sound.
*
*           This function can be used in conjunction with sceHeatWaveSoundGetSource() to loop through
*           all the sound sources that belong to a sound.
*
* @param[in]    hSoundHandle        The sound to query.
* @param[out]   pOutSourceCount     Receives the number of sound sources that belong to the sound if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundGetSource(), sceHeatWaveSoundCreateSource(), sceHeatWaveReleaseSource()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundGetSourceCount(SceHwSound hSoundHandle, int32_t *pOutSourceCount);

/**
* @brief    Gets information about a sound.
*
*           This consists of the number of sound effects that reference the sound
*           and the number of sound sources that belong to the sound. The information can be used
*           in conjunction with sceHeatWaveSoundGetSfx() and sceHeatWaveSoundGetSource() to loop through
*           a sound's sound effects and sound sources.
*
* @param[in]    hSoundHandle        The sound to query.
* @param[out]   pOutInfo            Receives the information about a sound.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see     sceHeatWaveSoundGetSfx(), sceHeatWaveSoundGetSource(), sceHeatWaveSoundGetSfxCount, sceHeatWaveSoundGetSourceCount()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundGetInfo(SceHwSound hSoundHandle, SceHwSoundInfo* pOutInfo);

/**
* @brief    Set a sound to loop on completion of all child sources.
*
*           A sound effect referencing this sound will repeat when all sources have completed playback.
*
* @param[in]    hSoundHandle    The sound to set.
* @param[in]    nLoopCount      Number of times to repeat. A value of 1 indicates that the sound should be played once and then repeated once.
*                               A value of 0 indicates that the sound should not be looped at all.
*
* @note     If <c><i>nLoopCount</i></c> is set to <c>knLoopInfinite</c>, the audio will continue to be output until <c>sceHeatWaveSfxStop()</c> is called.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSoundSetLoopCount(SceHwSound hSoundHandle, int32_t nLoopCount);

/**
* @brief    Sets the sound source to be a 3D or 2D emitter.
*
* @param[in]    hSourceHandle   The sound source to set.
* @param[in]    eType           The sound source emitter type.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSourceGetInfo(), sceHeatWaveSourceSetLoopCount(), sceHeatWaveSourceSetPan(), sceHeatWaveSourceSetType(), sceHeatWaveSourceSetCone(), sceHeatWaveSourceSetRolloffDistances()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetType(SceHwSource hSourceHandle, SceHwSourceType eType);

/**
* @brief    Sets a sound source to loop a number of times.
*
*           Each time the parent sound is played (via a sound effect), the sound source will be repeated this many additional times.
*
* @param[in]    hSourceHandle       The sound source to loop.
* @param[in]    nLoopCount          The number of times the sound source should repeat. A value of 1 indicates that the sound should be played once and then repeated once.
*                                   A value of 0 indicates that the sound should not be looped at all.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @note     If <c><i>nLoopCount</i></c> is set to <c>knLoopInfinite</c>, the audio will continue to be output until <c>sceHeatWaveSfxStop()</c> is called.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetLoopCount(SceHwSource hSourceHandle, int32_t nLoopCount);

/**
* @brief    Sets the distance over which the volume of a particular sound source will change (roll off) in relation to
*           the distance of the listener from the sound.
*
*           If the sound emission point is further than <c><i>fMaxDistance</i></c> from the listener,
*           the sound will not be further attenuated even if it moves farther away.
*           If the sound emission point is nearer than <c><i>fMinDistance</i></c> to the listener,
*           the sound will not get louder even if it gets closer. Sound roll-off occurs
*           between these two points and follows an inverse distance model where gain reduces
*           exponentially as distance increases.
*
*           The values set using this function work with the values set by sceHeatWaveSourceSetCone() to work out
*           the volume of a sound source when it is played as part of a sound effect. This means the volume of the
*           sound source in a sound effect will result from three things. The first is the listener's position inside/outside
*           of the sound effect's sound cone, which depends on values set by sceHeatWaveSourceSetCone() and the sound effect's direction.
*           The second is the distance of the listener from the sound effect's point of emission, which depends on values set by this function
*           and the sound effect's position. The third is the volume setting of the sound effect itself.
*
* @param[in]    hSourceHandle       The sound source to update the roll-off distance for.
* @param[in]    fMinDistance        The distance below which the volume will no longer be increased.
* @param[in]    fMaxDistance        The distance above which the volume will no longer be attenuated.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.  A value of <c>SCE_HEATWAVE_API_ERROR_OPERATION</c> will be returned if an attempt is made to
*           set a roll-off distance for a 2D sound source.
*
* @see      sceHeatWaveSourceSetCone()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetRolloffDistances(SceHwSource hSourceHandle, float32_t fMinDistance, float32_t fMaxDistance);

SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetDistanceFactor(SceHwSource hSourceHandle, float32_t fFactor);

/**
* @brief    Sets the 3D cone for a sound source.
*
*           The volume and high frequency content of the sound source audio varies according to the angle
*           between the direction the sound source is facing and the direction the listener is facing.
*           If this angle is greater than <c><i>fOutside</i></c>, the sound will not be further attenuated or experience
*           more high frequency filtering even if the angle increases. In such a case, the volume will remain at <c><i>fOutsideGain</i></c>
*           and the highest frequency cutoff point will remain at <c><i>fOutsideFreq</i></c>.
*           If the angle is less than <c><i>fInside</i></c>, the sound will not get louder or contain
*           more high frequencies even if the angle decreases. The amount of volume reduction and high frequency filtering linearly interpolates
*           between <c><i>fInside</i></c> and <c><i>fOutside</i></c>.
*
*           The values set using this function work with the values set by sceHeatWaveSourceSetRolloffDistances() to work out
*           the volume of a sound source when it is played as part of a sound effect. This means the volume of the
*           sound source in a sound effect will result from three things. The first is the listener's position inside/outside
*           of the sound effect's sound cone, which depends on values set by this function and the sound effect's direction.
*           The second is the distance of the listener from the sound effect's point of emission, which depends on values set by sceHeatWaveSourceSetRolloffDistances()
*           and the sound effect's position. The third is the volume setting of the sound effect itself.
*
* @param[in]    hSourceHandle       The sound source to update the 3D cone for.
* @param[in]    fInside             The inside angle of the cone. This should be between <c>kfSourceConeAngleMin</c> and <c>kfSourceConeAngleMax</c> degrees.
* @param[in]    fOutside            The outside angle of the cone. This should be between <c>kfSourceConeAngleMin</c> and <c>kfSourceConeAngleMax</c> degrees.
* @param[in]    fOutsideGain        The volume of the sound source at angles greater than <c><i>fOutside</i></c>. This should be between <c>kfSourceConeGainMin</c> and <c>kfSourceConeGainMax</c>.
* @param[in]    fOutsideFreq        The highest frequency cutoff point for the sound source at angles greater than <c><i>fOutside</i></c>. This should be between <c>kfSourceConeFreqMin</c> and <c>kfSourceConeFreqMax</c>.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes. A value of <c>SCE_HEATWAVE_API_ERROR_OPERATION</c> will be returned if an attempt is made to
*               set a sound cone for a 2D sound source.
*
* @see      sceHeatWaveSourceSetRolloffDistances()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetCone(SceHwSource hSourceHandle, float32_t fInside, float32_t fOutside, float32_t fOutsideGain, float32_t fOutsideFreq);

/**
* @brief        Sets the pan position for a sound source.
*
*               This function sets the position of a sound in a 2D stereo space.
*
* @param[in]    hSourceHandle       The sound source to apply the pan position to.
* @param[in]    fPan                The new pan position for the sound source, which should be between <c>kfSourcePanLeft</c> and <c>kfSourcePanRight</c>.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes. A value of <c>SCE_HEATWAVE_API_ERROR_OPERATION</c> will be returned if an attempt is made to
*               set the pan position for a 3D sound source.
*
* @see          sceHeatWaveSourceSetType()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetPan(SceHwSource hSourceHandle, float32_t fPan);

/**
* @brief        Sets the playback volume (gain) of a sound source.
*
* @param[in]    hSourceHandle   The sound source to set the volume for.
* @param[in]    fVolume         The new volume for the sound source. This must be a value between <c>kfSourceVolumeMin</c> and <c>kfSourceVolumeMax</c>.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetVolume(SceHwSource hSourceHandle, float32_t fVolume);

/**
* @brief        Sets the playback pitch for a sound source.
*
* @param[in]    hSourceHandle       The sound source to set the pitch multiplier for.
* @param[in]    fPitchMultiplier    The new pitch multiplier for the sound source. This must be a value between <c>kfSourcePitchMin</c> and <c>kfSourcePitchMax</c>.
*
* @return       See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceSetPitchMultiplier(SceHwSource hSourceHandle, float32_t fPitchMultiplier);

/**
* @brief    Gets the current information about a particular sound source.
*
*           This consists of the parent sound the sound source belongs to, the number of times the sound source
*           is set to loop on playback and whether the sound is 2D or 3D.
*
* @param[in]    hSourceHandle           The sound source to query.
* @param[out]   pOutInfo                Receives the current information about the sound source if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundCreateSource(), sceHeatWaveSoundGetSource(), sceHeatWaveSourceSetLoopCount(), sceHeatWaveSourceSetPan(), sceHeatWaveSourceSetType(), sceHeatWaveSourceSetCone(), sceHeatWaveSourceSetRolloffDistances()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSourceGetInfo(SceHwSource hSourceHandle, SceHwSourceInfo* pOutInfo);

/**
* @brief    Gets the current state of a particular sound effect.
*
*           A sound effect can either be ready to be played, playing or paused.
*
* @param[in]    hSfxHandle     The sound effect to query.
* @param[out]   pOutState      Receives the current state of the sound effect if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSfxPlay(), sceHeatWaveSfxStop(), sceHeatWaveSfxPause(), sceHeatWaveSfxResume()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxGetState(SceHwSfx hSfxHandle, SceHwSfxState *pOutState);

/**
* @brief    Gets the current information about a particular sound effect.
*
*           This consists of the sound that the sound effect references, the 3D position, velocity and direction,
*           and the current playback state.
*
* @param[in]    hSfxHandle          The sound effect to query.
* @param[out]   pOutInfo            Receives the current information about the sound effect if the operation was successful.
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*
* @see      sceHeatWaveSoundCreateSfx(), sceHeatWaveSoundGetSfx(), sceHeatWaveSfxSetPosition(), sceHeatWaveSfxSetVelocity(), sceHeatWaveSfxSetForward()
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSfxGetInfo(SceHwSfx hSfxHandle, SceHwSfxInfo* pOutInfo);

/**
* @brief    Gets a list of supported audio formats as MIME type strings
*
* @param[out]   ppMimeListOut   Array of MIME types as strings
* @param[out]   pSizeOut        Number of entries in the array
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetSupportedMimeTypes(const char **ppMimeListOut[], uint32_t *pSizeOut);

/**
* @brief    Set global value for speed of sound
*
* @param[in]    fSpeedOfSound   Value of speed of sound
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetSpeedOfSound(float32_t fSpeedOfSound);

/**
* @brief    Get global value for speed of sound
*
* @param[out]    pfSpeedOfSound   Value of speed of sound
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetSpeedOfSound(float32_t *pfSpeedOfSound);

/**
* @brief    Set global value for doppler effect scale factor
*
* @param[in]    fSpeedOfSound   Value of doppler effect scale factor
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetDopplerFactor(float32_t fDopplerFactor);

/**
* @brief    Set distance model
*
* @param[in]    nModel   Value of distance model
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveSetDistanceModel(int32_t nModel);

/**
* @brief    Get distance model
*
* @param[in]    pnModel   Value of distance model
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetDistanceModel(int32_t *pnModel);

/**
* @brief    Get global value for doppler effect scale factor
*
* @param[out]    pfDopplerFactor   Value of doppler effect scale factor
*
* @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
*/
SCE_HEATWAVE_FUNC SceHwError sceHeatWaveGetDopplerFactor(float32_t *pfDopplerFactor);

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif /* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

#endif /* _SCE_HEATWAVE_API_H_ */
