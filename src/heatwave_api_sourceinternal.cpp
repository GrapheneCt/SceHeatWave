/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_sourceinternal.h"
#include <float.h>
#include "heatwave_api_audiostream.h"
#include "heatwave_api_handleconvert.h"

namespace sce{
    namespace HeatWave{

        uint32_t SourceInternal::s_globalSourceCount = 0;

        SourceInternal::SourceInternal(uint32_t uIndex, SoundInternal *pParent)
            : m_dataStream(NULL),
            m_parent(pParent),
            m_index(uIndex),
            m_loopCount(0),
            m_minDistance(1.0f),
            m_maxDistance(FLT_MAX),
            m_distanceFactor(1.0f),
            m_insideAngle(0.0f),
            m_outsideAngle(360.0f),
            m_outsideGain(1.0f),
            m_outsideFreq(1.0f),
            m_pan(0.0f),
            m_volume(1.0f),
            m_pitch(1.0f),
            m_type(eSource3d)
        {
            m_id = _sceHeatWaveGenerateId();

            SourceInternal::s_globalSourceCount++;
        }

        SourceInternal::~SourceInternal()
        {
            release();

            SourceInternal::s_globalSourceCount--;
        }

        void SourceInternal::release()
        {
            if (m_dataStream != NULL)
            {
                delete m_dataStream;
                m_dataStream = NULL;
            }
        }

        AudioStream* SourceInternal::getAudioStream() const
        {
            return m_dataStream;
        }

        uint32_t SourceInternal::getIndex() const
        {
            return m_index;
        }

        SoundInternal* SourceInternal::getParentSound()
        {
            return m_parent;
        }

        int32_t& SourceInternal::loopCount()
        {
            return m_loopCount;
        }

        const int32_t& SourceInternal::loopCount() const
        {
            return m_loopCount;
        }

        float32_t& SourceInternal::minDistance()
        {
            return m_minDistance;
        }

        const float32_t& SourceInternal::minDistance() const
        {
            return m_minDistance;
        }

        float32_t& SourceInternal::maxDistance()
        {
            return m_maxDistance;
        }

        const float32_t& SourceInternal::maxDistance() const
        {
            return m_maxDistance;
        }

        float32_t& SourceInternal::distanceFactor()
        {
            return m_distanceFactor;
        }

        const float32_t& SourceInternal::distanceFactor() const
        {
            return m_distanceFactor;
        }

        float32_t& SourceInternal::insideAngle()
        {
            return m_insideAngle;
        }

        const float32_t& SourceInternal::insideAngle() const
        {
            return m_insideAngle;
        }

        float32_t& SourceInternal::outsideAngle()
        {
            return m_outsideAngle;
        }

        const float32_t& SourceInternal::outsideAngle() const
        {
            return m_outsideAngle;
        }

        float32_t& SourceInternal::outsideGain()
        {
            return m_outsideGain;
        }

        const float32_t& SourceInternal::outsideGain() const
        {
            return m_outsideGain;
        }

        float32_t& SourceInternal::outsideFreq()
        {
            return m_outsideFreq;
        }

        const float32_t& SourceInternal::outsideFreq() const
        {
            return m_outsideFreq;
        }

        float32_t& SourceInternal::pan()
        {
            return m_pan;
        }

        const float32_t& SourceInternal::pan() const
        {
            return m_pan;
        }

        float32_t& SourceInternal::volume()
        {
            return m_volume;
        }

        const float32_t& SourceInternal::volume() const
        {
            return m_volume;
        }

        float32_t& SourceInternal::pitch()
        {
            return m_pitch;
        }

        const float32_t& SourceInternal::pitch() const
        {
            return m_pitch;
        }

        SceHwSourceType& SourceInternal::type()
        {
            return m_type;
        }

        const SceHwSourceType& SourceInternal::type() const
        {
            return m_type;
        }

        int16_t SourceInternal::id() const
        {
            return m_id;
        }

        /**
        * @brief Return the duration in milliseconds.
        *
        * @return The millisecond duration.
        */
        int32_t SourceInternal::duration() const
        {
            const AudioStream *kpStream = getAudioStream();

            if (kpStream == NULL)
            {
                return 0;
            }

            return kpStream->getDuration();
        }

    } /* HeatWave */
} /* sce */
