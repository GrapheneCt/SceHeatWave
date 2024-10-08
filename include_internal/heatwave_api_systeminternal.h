/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_systeminternal.h
*
* @brief Top level system for HeatWave. Runs the internal update thread, PlatformSynth setup\shutdown, etc.
*
* @history
*   1.2     Rename ::updateSurroundSettings() to updateParameters()
*           Change ::init() to only spawn ::update() on a thread if in loopback mode, otherwise ::update() will be performed as part of sceHeatWaveRender()
*           Add support for callbacks
*   1.4     Change m_callbackInfo to not be a pointer.
*           Fix incorrect 3dAudio object count passed in the callback.
*           Add m_activeSoundList of valid child SoundInternal handles to replace iteration over entire m_soundMap to find handles.
*   1.44    Add ::getCurrentTimeMs() to query the time elapsed (driven by the underlying audio output).
*/

#ifndef _SCE_HEATWAVE_API_SYSTEM_INTERNAL_H_
#define _SCE_HEATWAVE_API_SYSTEM_INTERNAL_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"
#include <list>

namespace sce{
    namespace HeatWave{

        class SoundInternal;
        class Mutex;
        class Thread;
        class PlatformSynth;
        class Panner;

        #define SCE_HEATWAVE_LOCK() SystemInternal::getInstance()->lock()
        #define SCE_HEATWAVE_UNLOCK() SystemInternal::getInstance()->unlock()

        class SystemInternal : public MemoryHandler
        {
        public:
            static SystemInternal* createInstance();
            static SystemInternal* getInstance();

            SystemInternal();
            virtual ~SystemInternal();

            bool isInitialised();
            bool init(PlatformSynth *pSynth);
            void release();

            void update(); ///< Called from the worker thread

            SceHwError updateParameters();

            bool createSound(SoundInternal **ppOutSoundHandle);
            bool releaseSound(SoundInternal *pSoundHandle);
            SoundInternal* getSound(uint32_t uIndex);
            int32_t getSoundCount();

            void lock();
            void unlock();

            bool isUpdateThreadRunning();

            PlatformSynth* getSynth() const;
            Panner* getPanner();

            SceHwError setCallback(SceHwSystemCallbackFunction pCallbackFunc, void *pUserData = NULL);

            SceHwError getCurrentTimeMs(int32_t *pnTimeOut) const;

        private:
            SceHwError processCallback();

            SoundInternal** m_soundMap;
            std::list<SoundInternal*> m_activeSoundList;
            uint32_t m_soundCount; ///< Cache is faster then iterating over m_soundMap

            Mutex* m_globalMutex; ///< Use a global mutex for now, simpler for the time been than locking individual parts as needed

            Thread* m_systemUpdateThread; ///< Run the system update on an internal thread

            SystemInternal(const SystemInternal& rhs); /* Dummies to prevent copy */
            SystemInternal& operator=(const SystemInternal& rhs);

            bool m_isInitialised;
            PlatformSynth* m_synth;
            Panner* m_panner;

            SceHwSystemCallbackFunction m_callbackFunc; ///< User supplied callback, used to notify user of internal changes
            void*                       m_callbackUserData;
            SceHwSystemCallbackInfo     m_callbackInfo;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SYSTEM_INTERNAL_H_ */
