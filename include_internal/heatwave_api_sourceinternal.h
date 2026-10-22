/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_sourceinternal.h
*
* @brief A Source of audio.  Owned by a Sound
*
*   SoundInternal ---> SourceInternal ----> AudioStream
*                        |----> SourceInternal
*
* @history
*   1.2     Add ::volume() and ::pitch().
*           Add ::id() to allow verification of handles.
*   1.44    Add ::duration() query the duration in millseconds.
*/

#ifndef _SCE_HEATWAVE_API_SOURCE_INTERNAL_H_
#define _SCE_HEATWAVE_API_SOURCE_INTERNAL_H_

#include "heatwave_api_types.h"
#include "heatwave_api_memory.h"

namespace sce{
    namespace HeatWave{

        class SoundInternal;
        class AudioStream;

        class SourceInternal : public MemoryHandler
        {
        public:
            SourceInternal(uint32_t uIndex, SoundInternal *pParent);
            ~SourceInternal();

            void release();
            template <class T> T* init();
            AudioStream* getAudioStream() const;

            uint32_t getIndex() const;
            SoundInternal* getParentSound();

            int32_t&       loopCount();
            const int32_t& loopCount() const;

            float32_t&       minDistance();
            const float32_t& minDistance() const;

            float32_t&       maxDistance();
            const float32_t& maxDistance() const;

            float32_t&       distanceFactor();
            const float32_t& distanceFactor() const;

            float32_t&       insideAngle();
            const float32_t& insideAngle() const;

            float32_t&       outsideAngle();
            const float32_t& outsideAngle() const;

            float32_t&       outsideGain();
            const float32_t& outsideGain() const;

            float32_t&       outsideFreq();
            const float32_t& outsideFreq() const;

            float32_t&       pan();
            const float32_t& pan() const;

            float32_t&       volume();
            const float32_t& volume() const;

            float32_t&       pitch();
            const float32_t& pitch() const;

            SceHwSourceType&       type();
            const SceHwSourceType& type() const;

            int16_t id() const;
            int32_t duration() const;

        private:
            AudioStream* m_dataStream;
            SoundInternal* m_parent;
            uint32_t m_index;

            int32_t m_loopCount;

            float32_t m_minDistance;     ///< eSource3d
            float32_t m_maxDistance;     ///< eSource3d
            float32_t m_distanceFactor;  ///< eSource3d

            float32_t m_insideAngle;  ///< eSource3d
            float32_t m_outsideAngle; ///< eSource3d
            float32_t m_outsideGain;  ///< eSource3d
            float32_t m_outsideFreq;  ///< eSource3d

            float32_t m_pan; ///< eSource2d

            float32_t m_volume;
            float32_t m_pitch;

            SceHwSourceType m_type;

            int16_t m_id;

            static uint32_t s_globalSourceCount;

            SourceInternal(const SourceInternal& rhs); /* Dummy to prevent copy */
            SourceInternal& operator=(const SourceInternal& rhs);
        };

        template <class T> T* SourceInternal::init()
        {
            release();

            m_dataStream = new T();
            return (T*)m_dataStream;
        }

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_SOURCE_INTERNAL_H_ */
