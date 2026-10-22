/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_soundinternal.h
*
* @brief Used for setting up a sound that is going to be played.
* A sound has a collection of SourceInternal (which represent a source of audio data).
* Sfx handles are used for the playback of a sound - allows multiple instances of the same sound
*
*   SfxInternal -----> SoundInternal ---> SourceInternal ----> DataStream
*   SfxInternal -------^                    |----> SourceInternal ----> DataStream
*
* @history
*   1.1     Explicitly use 0 for memset in SoundInternal::initialise() (NULL implicit conversion to int32_t)
*   1.2     ::updateSurroundSettings() renamed ::updateParameters()
*           Add ::id() to allow verification of handles
*   1.3     Add looping functionality.
*   1.4     Add m_activeSfxList of valid child SfxInternal handles to replace iteration over entire m_sfxMap to find handles.
*   1.44    Add ::getDuration() to query duration in millseconds.
*/

#ifndef _SCE_HEATWAVE_API_SOUND_INTERNAL_H_
#define _SCE_HEATWAVE_API_SOUND_INTERNAL_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"
#include "heatwave_api_sourceinternal.h"
#include <list>

namespace sce{
    namespace HeatWave{

        class SourceInternal;
        class SfxInternal;

        class SoundInternal : public MemoryHandler
        {
        public:
            SoundInternal(uint32_t uIndex);
            ~SoundInternal();

            bool initialise();

            uint32_t getIndex() const;

            SourceInternal* getChildSource(uint32_t uIndex) const;
            SfxInternal*    getSfxHandle(uint32_t uIndex);

            bool createSource(SourceInternal **ppOutSourceHandle);
            bool releaseSource(SourceInternal *pSourceHandle);
            uint32_t getSourceCount() const;

            bool createSfx(SfxInternal **ppOutSfxHandle);
            bool releaseSfx(SfxInternal *pSfxHandle);
            uint32_t getSfxCount();

            void update();
            SceHwError updateParameters();

            int16_t id() const;

            SceHwError setLoopCount(int32_t nLoopCount);
            SceHwError getLoopCount(int32_t *pnLoopCountOut) const;

            SceHwError getDuration(int32_t* pnDurationOut) const;

        private:
            uint32_t m_index;

            SourceInternal** m_sourceMap;
            SfxInternal**    m_sfxMap;
            std::list<SfxInternal*> m_activeSfxList;

            uint32_t m_sfxCount; ///< Cache is faster then iterating over the maps and counting used slots
            uint32_t m_sourceCount;

            int16_t m_id;
            int32_t m_nLoopCount;

            static uint32_t s_globalSoundCount;

            SoundInternal(const SoundInternal& rhs); /* Dummies to prevent copy */
            SoundInternal& operator=(const SoundInternal& rhs);
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SOUND_INTERNAL_H_ */
