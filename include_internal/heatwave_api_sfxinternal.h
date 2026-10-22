/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_sfxinternal.h
*
* @brief Active instance of a SoundInternal used for playback
*
*   Multiple SfxInternal can point to the same SoundInternal
*   SfxInternal has a collection of VoiceInternal (which are basically instances of SourceInternal)
*
*   SfxInternal -----> SoundInternal
*   SfxInternal -------^
*
*   SfxInternal ---> VoiceInternal ---> SourceInternal
*                      |----> VoiceInternal ---> SourceInternal
*
* @history
*   1.2     Add SceHwSfxPlayParam parameter to ::play().
*           ::updateSurroundSettings() renamed ::updateParameters().
*           Add support for callbacks on change of state.
*           Add ::id() to allow verification of handles.
*   1.3     Add support for looping on completion of playback of all ChildVoices.
*           Add ::initialise and ::release to perform resource alloaction and cleanup.
*           Change ::play and ::stop to not perform initialisation or release.
*           Add ::s_defaultPlaybackParam as default playback parameters.
*   1.4     Fix callback using incorrect SceHwSfx handle.
            Change m_callbackInfo to not be a pointer.
*   1.44    Add support to query the time elapsed during playback.
*           Add support to seek playback time.
*           Fix SceHwSfxState is eStateReady when all ChildVoices have completed, not when the last buffer of data is passed to the voice.
*/

#ifndef _SCE_HEATWAVE_API_SFX_INTERNAL_H_
#define _SCE_HEATWAVE_API_SFX_INTERNAL_H_

#include "heatwave_api.h"
#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class SoundInternal;
        class VoiceInternal;

        class SfxInternal : public MemoryHandler
        {
        public:
            SfxInternal(uint32_t uIndex, SoundInternal *pParent);
            ~SfxInternal();

            SceHwError initialise(SceHwSfxPlayParam *pParam);
            SceHwError release();

            uint32_t getIndex() const;
            SoundInternal* getParentSound();

            SceHwError play();
            SceHwError stop();

            bool pause();
            bool resume();

            SceHwError getState(SceHwSfxState *pState) const;

            void update();
            SceHwError updateParameters();

            bool setPosition(SceHwVector vPosition);
            bool setVelocity(SceHwVector vVelocity);
            bool setForward(SceHwVector vForward);

            void getPosition(SceHwVector *pPositionOut);
            void getVeloctiy(SceHwVector *pVelocityOut);
            void getForward(SceHwVector  *pForwardOut);

            bool setVolume(float32_t fVol);
            float32_t getVolume() const;

            bool setPitch(float32_t fPitchMul);
            float32_t getPitch() const;

            static SceHwError setCallback(SceHwSfxCallbackFunction pCallbackFunc, void *pUserData);

            int16_t id() const;
            int32_t getElapsedTime() const;
            SceHwError seek(int32_t nTimeMs);

            static SceHwSfxPlayParam s_defaultPlaybackParam;

        private:
            typedef struct ChildVoice : public MemoryHandler
            {
                VoiceInternal* voice;
                SceHwSourceType voiceType;
                ChildVoice* next;
            } ChildVoice;

            void addChildVoice(VoiceInternal *pVoice);

            SceHwError processCallback();

            SoundInternal* m_parent;
            uint32_t m_index;

            SceHwVector m_position;
            SceHwVector m_velocity;
            SceHwVector m_forward;

            float32_t m_volume;
            float32_t m_pitchMultiplier;

            ChildVoice* m_voices;

            static uint32_t s_globalSfxCount;

            SceHwSfxCallbackInfo m_callbackInfo;

            int16_t m_id;
            int32_t m_nLoopsRemaining;

            bool m_bIsInitialised;

            int32_t m_nUpdateTime;
            int32_t m_nElapsedTime;
            int32_t m_nSeekOffset;

            SceHwSfxState m_state;

            static SceHwSfxCallbackFunction m_callbackFunc; ///< User supplied callback, used to notify user of internal changes
            static void* m_callbackUserdata;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SFX_INTERNAL_H_ */
