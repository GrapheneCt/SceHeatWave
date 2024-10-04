/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_types.h
*
* @brief Types used by HeatWave.
*
* @history
*   1.2     Add SceHwSfxPlayParam.
*           Add SceHwSfxCallbackInfo, SceHwSfxCallbackFunction, SceHwSystemCallbackInfo & SceHwSystemCallbackFunction.
*           Add fGain and fPitch to SceHwSourceInfo.
*           Add SceHwSystemOutputType and SceHwInfo.
*   1.3     Add ScewHwSourceRuntimeStreamCallbackType, SceHwSourceRuntimeStreamCallbackInfo,
*               SceHwSourceRuntimeStreamParam & SceHwSourceRuntimeStreamCallbackFunction.
*   1.4     Change handle type from uint32_t to uint64_t.
*           Add SCE_HEATWAVE_API_OUTPUT_XXX to represent the destination output.
*           Add SceHwInfo.bSupportedPorts.
*           Add SceHwSfxPlayParam.nDestination to allow user to specify the output destination of the SceHwSfx.
*           Deprecate SceHwSfxPlayParam.bIsAudio3d.
*   1.44    Add SceHwSoundInfo.nDurationMs.
*           Add SceHwSfxInfo.nTimeElapsedMs.
*/

#ifndef _SCE_HEATWAVE_TYPES_H_
#define _SCE_HEATWAVE_TYPES_H_

#include <cstddef>
#include <stddef.h>
#include <stdint.h>

/**
* @brief    Defines a 32-bit float.
*/
typedef float  float32_t;

/**
* @brief    Defines a HeatWave error type. This is used as the return type by all HeatWave API functions
*
* @note     An <c>SceHwError</c> could hold a result indicating success.                                                   
*/
typedef int32_t SceHwError;

/**
* @brief    Base HeatWave API handle type.
*
*           All handles used by the HeatWave API derive from SceHwHandle.
*/
typedef uint64_t SceHwHandle;

/**
* @brief    Defines a <b>sound source</b>.
*
*           This will be an audio file. Emitter cones and volume roll-off distances can be set per sound source.
*           Currently, uncompressed 16 bit WAV files are the only supported audio file format.  
*
* @see      sceHeatWaveSoundCreateSource()
*/
typedef SceHwHandle SceHwSource;

/**
* @brief    Defines a single <b>sound</b>, which is a collection of sound sources that are played together.
*
*           A sound can be referenced by multiple sound effects, which enables simultaneously occurring events to all make use
*           of a single sound. In such cases, an instance of a sound being played can overlap in time with other instances
*           of it being played.
*
*           Because a sound can play several sound sources in unison, it also enables more than one sound source to be heard
*           in response to a particular event occurring. The sound sources belonging to a sound can be a mixture of 2D and 3D.     
*
* @see      sceHeatWaveCreateSound()
*/
typedef SceHwHandle SceHwSound;

/**
* @brief    Defines a <b>sound effect</b> (an instance of a sound) to use for playback.
*
*           A sound effect needs to be created before any audio can actually be heard and manipulated
*           (setting the volume of the audio, for example). Multiple sound effects can reference a single sound
*           and these sound effects could all play that sound simultaneously.
*
* @see      sceHeatWaveSoundCreateSfx()
*/
typedef SceHwHandle SceHwSfx;

/**
* @brief    Defines the possible playback states of a sound effect.
*
* @see      sceHeatWaveSfxGetState()
*/
typedef enum SceHwSfxState
{
    kSfxState_Ready     = 0,    ///< The sound effect is ready to be played.
    kSfxState_Playing   = 1,    ///< The sound effect is outputting audio data.
    kSfxState_Paused    = 2     ///< The sound effect has been paused.
} SceHwSfxState;

/**
* @brief Defines the ways in which a sound source can be played.
*
* It is possible to pan 2D sound sources while 3D sound sources can have a sound cone and roll-off distance set.
* The sound sources belonging to a sound can be a mixture of 2D and 3D.     
* 
* @see sceHeatWaveSourceGetInfo(), sceHeatWaveSourceSetType(), sceHeatWaveSourceSetPan(), sceHeatWaveSourceSetCone(), sceHeatWaveSourceSetRolloffDistances() 
*/
typedef enum SceHwSourceType
{
    eSource2d = 0,  ///< Play the sound source as a 2D sound at the listener position.
    eSource3d = 1   ///< Play the sound source with 3D positional information.
} SceHwSourceType;

/**
* @brief Defines triggers of the system callback.
*
* Indicates the reason for execution of <c>SceHwSystemCallbackFunction</c>.
*
* @see sceHeatWaveSetCallback()
*/
typedef enum SceHwSystemCallbackType
{
    eSysCbType_A3dObjectCount = 0   ///< The number of active Audio3d objects has changed.
} SceHwSystemCallbackType;

/**
* @brief Defines the triggers of the <c>SceHwSource</c> runtime audio callback.
*
* Defines the reason for the execution of <c>SceHwSystemCallbackFunction</c>.
*
* @see sceHeatWaveSourceLoadRuntimeStream
*/
typedef enum ScewHwSourceRuntimeStreamCallbackType
{
    eStreamCreate = 0,  ///< Create an instance of the stream.
    eStreamRelease,     ///< Release an instance of the stream.
    eStreamRender,      ///< Render a buffer of audio data.
    eStreamReset        ///< Reset the stream to initial state.
} ScewHwSourceRuntimeStreamCallbackType;

/**
* @brief Defines triggers of an Sfx callback
*
* Indicates the reason for execution of <c>SceHwSfxCallbackFunction</c>.
*
* @see sceHeatWaveSetSfxCallback()
*/
typedef enum SceHwSfxCallbackType
{
    eSfxCbType_StateChange = 0  ///< The state of the <c>SceHwSfx</c> has changed.
} SceHwSfxCallbackType;

/**
* @brief Defines the audio outputs that HeatWave can use.
*
* @see sceHeatWaveGetInfo()
*/
typedef enum SceHwSystemOutputType
{
    eSysOutput_Ngs          = 0, ///< PlayStation&#174;Vita NGS Synth.
    eSysOutput_Multistream  = 1, ///< PlayStation&#174;3 Multistream.
    eSysOutput_Ngs2         = 2, ///< PlayStation&#174;4 NGS2 Synth.
    eSysOutput_XAudio2      = 3, ///< Win XAudio.
    eSysOutput_Audio3d      = 4, ///< PlayStation&#174;4 Audio3d.
    eSysOutput_UserRender   = 5  ///< Output is performed via sceHeatWaveRender().
} SceHwSystemOutputType;

/**
* @brief Main output.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_MAIN       (0)

/**
* @brief Auxiliary output.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_AUX        (1)

/**
* @brief Pad speaker of Player 1.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_PAD_1      (2)

/**
* @brief Pad speaker of Player 2.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_PAD_2      (3)

/**
* @brief Pad speaker of Player 3.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_PAD_3      (4)

/**
* @brief Pad speaker of Player 4.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_PAD_4      (5)

/**
* @brief Audio3d object.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_AUDIO3D    (6)

/**
* @brief Number of supported outputs.
*
* @see SceHwSfxPlayParam
*/
#define SCE_HEATWAVE_API_OUTPUT_MAX        (7)

/**
* @brief    Defines a coordinate or vector in 3D space.
*
* @see      sceHeatWaveSetListener(), sceHeatWaveSfxSetPosition(), sceHeatWaveSfxSetVelocity(), sceHeatWaveSfxSetForward()
*/
typedef struct SceHwVector
{
    float32_t fX; ///< The x axis component.
    float32_t fY; ///< The y axis component.
    float32_t fZ; ///< The z axis component.
} SceHwVector;

/**
* @brief Represents information about HeatWave.
*
* @see sceHeatWaveGetInfo()
*/
typedef struct SceHwInfo
{
    SceHwSystemOutputType eOutputType; ///< The type of audio output in use.
    bool bSupportedPorts[SCE_HEATWAVE_API_OUTPUT_MAX]; ///< The currently supported audio output destinations.
} SceHwInfo;

/**
* @brief Represents information about a sound.
*
* @see sceHeatWaveSoundGetInfo()
*/
typedef struct SceHwSoundInfo
{
    uint32_t    uNumSfx;           ///< The number of sound effects that reference the sound. 
    uint32_t    uNumSources;       ///< The number of sound sources that belong to the sound.
    int32_t     nDurationMs;        ///< The duration in milliseconds.
} SceHwSoundInfo;

/**
* @brief Represents information about a sound source.
*
* @see sceHeatWaveSourceGetInfo()
*/
typedef struct SceHwSourceInfo
{
    SceHwSound hParentSound;    ///< The parent sound that the sound source belongs to.
    int32_t nLoopCount;         ///< The number of times the audio data is set to loop on playback.
    SceHwSourceType eType;      ///< Specifies whether this sound source is a 2D or 3D sound emitter.
    float32_t fGain;            ///< The volume multiplier of the source.
    float32_t fPitch;           ///< The pitch multiplier of the source.
} SceHwSourceInfo;

/**
* @brief Represents information about a sound effect.
*
* @see sceHeatWaveSfxGetInfo()
*/
typedef struct SceHwSfxInfo
{
    SceHwSound hParentSound;    ///< The sound that the sound effect references.
    SceHwVector vPosition;      ///< The current 3D position.
    SceHwVector vVelocity;      ///< The current 3D velocity.
    SceHwVector vForward;       ///< The current 3D forward direction.
    SceHwSfxState eSfxState;    ///< The current playback state.
    int32_t nTimeElapsedMs;       ///< The playback time elapsed in milliseconds.
} SceHwSfxInfo;

/**
* @brief Information for playback as an Audio3d object.
*
* @see sceHeatWaveSfxPlay()
*/
typedef struct SceHwSfxPlayParam
{
    bool bIsAudio3d; ///< DEPRECATED from v1.4 - <c><i>nDestination</i></c> must be used instead.
    int32_t nAudio3dPriority; ///< Priority of the <c>SceHwSfx</c> Audio3d object.
    int32_t nDestination; ///< The output destination for the <c>SceHwSfx</c>.  Must be one of the SCE_HEATWAVE_API_OUTPUT_XXX values.
} SceHwSfxPlayParam;

/**
* @brief Information about a runtime stream.
*
* @see sceHeatWaveSourceLoadRuntimeStream()
*/
typedef struct SceHwSourceRuntimeStreamParam
{
    int32_t nChannelCount; ///< The channel count of the audio data.
    int32_t nSampleRate; ///< The sample rate of the audio data.
} SceHwSourceRuntimeStreamParam;

/**
* @brief Defines a memory allocation function.
*
* @param    size    The size of the memory to allocate.
*
* @return           A pointer to the newly allocated memory if the operation was successful; otherwise <c>NULL</c> is returned.
*
* @see              SceHwMemoryFree()
*/
typedef void* (*SceHwMemoryAlloc)(size_t size);

typedef void* (*SceHwMemoryRealloc)(void *ptr, size_t size);

typedef void* (*SceHwMemoryCalloc)(size_t num, size_t size);

/**
* @brief Defines an aligned memory allocation function.
*
* @param    align   The alignment of the memory to allocate.
* @param    size    The size of the memory to allocate.
*
* @return           A pointer to the newly allocated memory if the operation was successful; otherwise <c>NULL</c> is returned.
*
* @see              SceHwMemoryFree()
*/
typedef void* (*SceHwMemoryAlignAlloc)(size_t align, size_t size);

/**
* @brief Defines a memory freeing function.
*
* @param    ptr     A pointer to a previously allocated memory block.   
*
* @see              SceHwMemoryAlloc()
*/
typedef void  (*SceHwMemoryFree)(void* ptr);

/**
* @brief    Represents user-provided memory functions for HeatWave to use.
*
*           Each member of a <c>SceHwMemoryFunctions</c> object must point to a valid custom function before
*           the object is passed as an argument to sceHeatWaveSetMemoryHandlers().  
*
* @see      sceHeatWaveSetMemoryHandlers()
*/
typedef struct SceHwMemoryFunctions
{
    SceHwMemoryAlloc        pAlloc;         ///< A user defined memory allocation function.
    SceHwMemoryRealloc      pRealloc;       ///< A user defined memory reallocation function.
    SceHwMemoryCalloc       pCalloc;        ///< A user defined memory allocation function with memset to 0.
    SceHwMemoryAlignAlloc   pAlignedAlloc;  ///< A user defined aligned memory allocation function.
    SceHwMemoryFree         pFree;          ///< A user defined memory freeing function.
} SceHwMemoryFunctions;

/**
* @brief Defines a custom <c>fopen()</c> handler.
*
* @param strFilePath    The full path of the file to open.
* @param strMode        The mode to open the file in.
*
* @return               A pointer to the opened file if the operation was successful; otherwise <c>NULL</c> is returned.
*
* @see                  SceHwFileClose()
*/
typedef void* (*SceHwFileOpen)(const char *strFilePath, const char *strMode);

/**
* @brief Defines a custom <c>fclose()</c> handler.
*
* @param pStream        The file stream to close.
*
* @return               A value of 0 is returned if the operation was successful.
*
* @see                  SceHwFileOpen()
*/
typedef int32_t (*SceHwFileClose)(void *pStream);

/**
* @brief Defines a custom <c>fseek()</c> handler.
*
* @param pStream        The file stream to seek within.
* @param uOffset        The number of bytes to offset from <c><i>iOrigin</i></c>.
* @param iOrigin        The position in the file from which offset is applied. This can be set to <c><i>SEEK_SET</i></c>, <c><i>SEEK_CUR</i></c> or <c><i>SEEK_END</i></c>.
*
* @return 0             A value of 0 is returned if the operation was successful.
*
* @see                  SceHwFileOpen(), SceHwFileTell()
*/
typedef int32_t (*SceHwFileSeek)(void *pStream, uint32_t uOffset, int32_t iOrigin);

/**
* @brief Defines a custom file <c>ftell()</c> handler.
*
* @param pStream        The file stream to report the position of.
*
* @return               The current position of <c><i>pStream</i></c>. A value of -1L indicates an error has occurred.
*
* @see                  SceHwFileOpen(), SceHwFileSeek()
*/
typedef int32_t (*SceHwFileTell)(void *pStream);

/**
* @brief Defines a custom <c>fread()</c> handler.
*
* @param pPtr       The memory block to read data into.
* @param uSize      The size of an element in bytes.
* @param uCount     The number of elements to read.
* @param pStream    The file stream to read the elements from.
*
* @return           The number of elements read.
*
* @see              SceHwFileOpen(), SceHwFileSeek()
*/
typedef uint32_t (*SceHwFileRead)(void* pPtr, uint32_t uSize, uint32_t uCount, void* pStream);

/**
* @brief    Represents user overrides for the file operation functions that the HeatWave library uses.
*
*           Each member of a <c>SceHwFileFunctions</c> object must point to a valid custom function before the object is
*           passed as an argument to sceHeatWaveSetFileHandlers().  
*
* @see      sceHeatWaveSetFileHandlers()
*/
typedef struct SceHwFileFunctions
{
    SceHwFileOpen  pOpen;       ///< A user defined version of <c>fopen()</c>.
    SceHwFileClose pClose;      ///< A user defined version of <c>fclose()</c>.
    SceHwFileSeek  pSeek;       ///< A user defined version of <c>fseek()</c>.
    SceHwFileTell  pTell;       ///< A user defined version of <c>ftell()</c>.
    SceHwFileRead  pRead;       ///< A user defined version of <c>fread()</c>.
} SceHwFileFunctions;

/**
* @brief    Represents information about a memory location in which an audio file has been loaded.
*
*           Before a call is made to sceHeatWaveSourceLoadMemory(), memory must be allocated for
*           the <c><i>pBuffer</i></c> member and then filled with the contents of an audio file. 
*           
*           Currently, uncompressed 16-bit WAV files are the only supported format. 
*
* @see      sceHeatWaveSourceLoadMemory()
*/
typedef struct SceHwMemoryFileInfo
{
    char*       pBuffer;        ///< A pointer to the memory containing the audio data.
    uint32_t    uBufferLen;     ///< The length in bytes of <c><i>pBuffer</i></c>.
} SceHwMemoryFileInfo;

/**
* @brief    Represents the initialization settings for HeatWave library.
*
* @note     Loopback mode is not supported on the Windows platform.
*
* @see      sceHeatWaveInit()
*/
typedef struct SceHwInitParams
{
    bool bLoopBack;                ///< A flag that specifies whether to use loopback mode. When in this mode, the Heatwave library will not feed data directly into the audio output, and sceHeatWaveRender() must be called to render the data into a suitable buffer.
#if defined __psp2__
    int32_t nMaxChannelNumMp3;     ///< Maximum number of simultaneously decoded MP3 channels, limited by 8
    int32_t nMaxChannelNumAt9;     ///< Maximum number of simultaneously decoded ATRAC9 channels, limited by 16
#endif
} SceHwInitParams;

/**
* @brief    Information structure passed to user callback function.
*
* @see      sceHeatWaveSetSfxCallback()
*/
typedef struct SceHwSfxCallbackInfo
{
    SceHwSfx        hSfxHandle;             ///< The <c>SceHwSfx</c> which triggered the callback.
    SceHwSfxState   currentState;           ///< Current state of <c>hSfxHandle</c>.
    SceHwSfxState   previousState;          ///< Previous state of <c>hSfxHandle</c>.
} SceHwSfxCallbackInfo;

/**
* @brief    Defines the user callback function.
*
* @param[in] pCallbackInfo  Callback information.
* @param[in] pUserData      The user data set by sceHeatWaveSetSfxCallback().
* @param[in] reason         <c>SceHwSfxCallbackType</c> specifying why the callback is being issued.
*
* @see      sceHeatWaveSetSfxCallback()
*/
typedef void(*SceHwSfxCallbackFunction)(const SceHwSfxCallbackInfo *pCallbackInfo, void *pUserData, SceHwSfxCallbackType reason);

/**
* @brief    Information structure passed to user callback function.
*
* @see      sceHeatWaveSetCallback()
*/
typedef struct SceHwSystemCallbackInfo
{
    uint32_t    uCurrentA3dObjectCount;         ///< Number of currently active libAudio3d objects.
    uint32_t    uPreviousA3dObjectCount;        ///< Previous number of active libAudio3d objects.
} SceHwSystemCallbackInfo;

/**
* @brief    Defines the user callback function.
*
* @param[in] pCallbackInfo  Callback information.
* @param[in] pUserData      The user data set by sceHeatWaveSetCallback().
* @param[in] reason         <c>SceHwSystemCallbackType</c> specifying why the callback is being issued.
*
* @see      sceHeatWaveSetCallback()
*/
typedef void(*SceHwSystemCallbackFunction)(const SceHwSystemCallbackInfo *pCallbackInfo, void *pUserData, SceHwSystemCallbackType reason);

/**
* @brief    Information structure passed to <c>SceHwSourceRuntimeStreamCallbackFunction</c>.
*
* @see      sceHeatWaveSourceLoadRuntimeStream()
*/
typedef struct SceHwSourceRuntimeStreamCallbackInfo
{
    char *pcBuffer;             ///< Buffer to receive audio data.
    int32_t nBufferSizeBytes;   ///< Size of buffer in bytes.
    int32_t nBytesWrittenOut;   ///< The amount of data written to the buffer by the callback.
} SceHwSourceRuntimeStreamCallbackInfo;

/**
* @brief        Defines the user callback used to generate audio data at runtime.
*
* @param[in]    pCallbackInfo  Callback information. The <c>nSampleRate</c> and <c>nChannelCount</c> should be set by the user when <c><i>reason</i></c> is <c>eStreamCreate</c>.
* @param[in]    pUserData      The user data set by sceHeatWaveSourceLoadRuntimeStream().
* @param[in]    reason         <c>ScewHwSourceRuntimeStreamCallbackType</c> specifying why the callback was issued.
*
* @see          sceHeatWaveSourceLoadRuntimeStream()
*/
typedef void(*SceHwSourceRuntimeStreamCallbackFunction)(SceHwSourceRuntimeStreamCallbackInfo *pCallbackInfo, void *pUserData, ScewHwSourceRuntimeStreamCallbackType reason);

#define SCE_HEATWAVE_API_DISTANCE_MODEL_INVERSE_CLAMPED       (7)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_INVERSE               (6)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_LINEAR_CLAMPED        (5)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_LINEAR                (4)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_EXPONENT_CLAMPED      (3)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_EXPONENT              (2)
#define SCE_HEATWAVE_API_DISTANCE_MODEL_DISABLE               (1)
#endif /* _SCE_HEATWAVE_TYPES_H_ */
