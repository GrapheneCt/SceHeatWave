/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_soundinternal.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_sfxinternal.h"
#include <stdlib.h>
#include "heatwave_api_errors.h"

namespace sce{
    namespace HeatWave{

        uint32_t SoundInternal::s_globalSoundCount = 0;

        SoundInternal::SoundInternal(uint32_t uIndex)
            : m_index(uIndex),
            m_sourceMap(NULL),
            m_sfxMap(NULL),
            m_sfxCount(0),
            m_sourceCount(0),
            m_nLoopCount(0)
        {
            m_id = _sceHeatWaveGenerateId();
            SoundInternal::s_globalSoundCount++;
        }

        SoundInternal::~SoundInternal()
        {
            SfxInternal     *sfxhandle = NULL;
            SourceInternal  *childsource = NULL;

            /* Sfx handles */
            if (m_sfxMap != NULL)
            {
                for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
                {
                    sfxhandle = m_sfxMap[i];
                    if (sfxhandle != NULL)
                    {
                        delete sfxhandle;
                        m_sfxMap[i] = NULL;
                    }
                }
                MemoryHandler::freeCb(m_sfxMap);
                m_sfxMap = NULL;
            }
            m_activeSfxList.clear();

            /* Sources */
            if (m_sourceMap != NULL)
            {
                for ( uint32_t i = 0; i < _sceHeatWaveGetMaxSoundCount(); i++ )
                {
                    childsource = m_sourceMap[i];
                    if (childsource != NULL)
                    {
                        delete childsource;
                        m_sourceMap[i] = NULL;
                    }
                }
                MemoryHandler::freeCb(m_sourceMap);
                m_sourceMap = NULL;
            }

            SoundInternal::s_globalSoundCount--;
        }

        /**
        * @brief Allocate memory needed
        */
        bool SoundInternal::initialise()
        {
            m_sourceMap = (SourceInternal**)MemoryHandler::mallocCb(sizeof(SourceInternal*) * _sceHeatWaveGetMaxSourceCount());
            if (m_sourceMap == NULL)
            {
                return false;
            }
            MemoryHandler::memsetCb(m_sourceMap, 0, sizeof(SoundInternal*) * _sceHeatWaveGetMaxSourceCount());

            m_sfxMap = (SfxInternal**)MemoryHandler::mallocCb(sizeof(SfxInternal*) * _sceHeatWaveGetMaxSfxCount());
            if (m_sfxMap == NULL)
            {
                return false;
            }
            MemoryHandler::memsetCb(m_sfxMap, 0, sizeof(SfxInternal*) * _sceHeatWaveGetMaxSfxCount());

            return true;
        }

        /**
        * @brief Create a new source and store at the first available position in m_sourceMap
        * @note Have to iterate over m_sourceMap as cannot guarentee that create\release of sources are in order
        */
        bool SoundInternal::createSource(SourceInternal **ppOutSourceHandle)
        {
            int32_t newSourceID = -1;
            SourceInternal *sourceHandle = NULL;

            if (ppOutSourceHandle == NULL)
            {
                return false;
            }

            if (getSourceCount() >= _sceHeatWaveGetMaxSourceCount()) /* Is there space? */
            {
                return false;
            }

            /* Get an available slot for the new source */
            for (uint32_t i = 0; i < _sceHeatWaveGetMaxSoundCount(); i++)
            {
                if (m_sourceMap[i] == NULL)
                {
                    newSourceID = i;
                    break;
                }
            }

            if (newSourceID == -1)
            {
                return false; /* No space for sources in this sound */
            }

            sourceHandle = new SourceInternal(newSourceID, this);
            if (!sourceHandle)
            {
                return false;
            }

            m_sourceMap[newSourceID] = sourceHandle;
            *ppOutSourceHandle      = sourceHandle;

            m_sourceCount++;

            return true;
        }

        /**
        * @brief Check pSourceHandle is a child of this, and release any resources if it is
        */
        bool SoundInternal::releaseSource(SourceInternal *pSourceHandle)
        {
            if (pSourceHandle == NULL)
            {
                return false;
            }

            if (pSourceHandle->getIndex() > _sceHeatWaveGetMaxSourceCount())
            {
                return false;
            }

            if (m_sourceMap[pSourceHandle->getIndex()] != pSourceHandle) /* Make sure the source is a child of this */
            {
                return false;
            }

            /* stop any playing sfx that could be pointing to the source we are about to delete (would cause dangling pointers) */
            for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
            {
                if (m_sfxMap[i] != NULL)
                {
                    m_sfxMap[i]->stop();
                }
            }

            m_sourceMap[pSourceHandle->getIndex()] = NULL;
            delete pSourceHandle;

            m_sourceCount--;

            return true;
        }

        /**
        * @brief Number of used source slots - m_sourceMap
        */
        uint32_t SoundInternal::getSourceCount() const
        {
            return m_sourceCount;
        }

        /**
        * @brief Find the first available slot in m_sfxMap and create a new sfx in it
        * @note Have to iterate over m_sfxMap as cannot guarentee that create\release of sfx instances are in order
        */
        bool SoundInternal::createSfx(SfxInternal **ppOutSfxHandle)
        {
            int32_t newSfxID = -1;
            SfxInternal *sfxHandle = NULL;

            if (ppOutSfxHandle == NULL)
            {
                return false;
            }

            if (getSfxCount() >= _sceHeatWaveGetMaxSfxCount()) /* Is there space? */
            {
                return false;
            }

            /* Get an available slot for the sfx handle */
            for (uint32_t i = 0; i < _sceHeatWaveGetMaxSfxCount(); i++)
            {
                if (m_sfxMap[i] == NULL)
                {
                    newSfxID = i;
                    break;
                }
            }

            if (newSfxID == -1)
            {
                return false; /* No space for sfx handles to this sound */
            }

            sfxHandle = new SfxInternal(newSfxID, this);
            if (sfxHandle == NULL)
            {
                return false;
            }

            m_sfxMap[newSfxID] = sfxHandle;
            *ppOutSfxHandle    = sfxHandle;
            m_activeSfxList.push_back(sfxHandle);

            m_sfxCount++;

            return true;
        }

        /**
        * @brief Check pSfxHandle is a child of this, and release any resources if it is
        */
        bool SoundInternal::releaseSfx(SfxInternal *pSfxHandle)
        {
            if (pSfxHandle == NULL)
            {
                return false;
            }

            if (pSfxHandle->getIndex() > _sceHeatWaveGetMaxSfxCount())
            {
                return false;
            }

            if (m_sfxMap[pSfxHandle->getIndex()] != pSfxHandle) /* Make sure pSfxHandle belongs to this */
            {
                return false;
            }

            m_sfxMap[pSfxHandle->getIndex()] = NULL;
            m_activeSfxList.remove(pSfxHandle);
            delete pSfxHandle;

            m_sfxCount--;

            return true;
        }

        /**
        * @brief Number of used sfx slots - m_sfxMap
        */
        uint32_t SoundInternal::getSfxCount()
        {
            return m_sfxCount;
        }

        /**
        * @brief update all child sfx.
        * Called from the internal update thread
        */
        void SoundInternal::update()
        {
            SfxInternal *current = NULL;

            for (std::list<SfxInternal*>::iterator it = m_activeSfxList.begin(); it != m_activeSfxList.end(); it++)
            {
                current = *it;
                current->update();
            }
        }

        /**
        * @brief Update the parameters of all child sfx.
        *
        * Recalculate the internal parameters of all child sfx (for instance if the listener position changes).
        */
        SceHwError SoundInternal::updateParameters()
        {
            SceHwError  res = SCE_HEATWAVE_API_OK;
            SfxInternal *current = NULL;

            for (std::list<SfxInternal*>::iterator it = m_activeSfxList.begin(); it != m_activeSfxList.end(); it++)
            {
                current = *it;
                res = current->updateParameters();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

        int16_t SoundInternal::id() const
        {
            return m_id;
        }

        /**
        * @brief    Set the sound to repeat on completion of all child sources
        *
        *           Sets the number of times a SceHwSound will be repeated on completion
        *           of all child SceHwSource.
        *
        * @param[in]    nLoopCount  The number of times to repeat.  A value of 1 indicates that the sound should be played once and then repeated once.
        *                           A value of 0 indicates that the sound should not be looped at all.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        *
        * @note     If <c><i>nLoopCount</i></c> is set to <c>knLoopInfinite</c>, the audio will continue to be output until <c>sceHeatWaveSfxStop()</c> is called.
        */
        SceHwError SoundInternal::setLoopCount(int32_t nLoopCount)
        {
            if (nLoopCount < knLoopInfinite)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            m_nLoopCount = nLoopCount;
            return SCE_HEATWAVE_API_OK;
        }
        /**
        * @brief    Retrieve how many times the Sound should repeat on completion of all child sources.
        *
        *           The number of times the Sound should repeat once all the child Sources have finished playback.
        *
        * @param[out]   pnLoopCountOut  Receives the repeat count for the Sound.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SoundInternal::getLoopCount(int32_t *pnLoopCountOut) const
        {
            if (pnLoopCountOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pnLoopCountOut = m_nLoopCount;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Calculate the millisecond duration of the SceHwSound.
        *
        * Find the child SceHwSource with the longest duration (accounting for loop counts),
        * then calculate a duration using the longest child SceHwSource and the loop count
        * of the SceHwSound.
        *
        * @param[out] pnDuration Variable to receive the millisecond duration.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SoundInternal::getDuration(int32_t* pnDurationOut) const
        {
            int32_t nMaxSourceDuration = 0;
            int32_t nCurrentSourceDuration = 0;
            if (pnDurationOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            // If the SceHwSound loops infinitely we know the duration has to also be infinite.
            if (m_nLoopCount == knLoopInfinite)
            {
                *pnDurationOut = knDurationInfinite;
                return SCE_HEATWAVE_API_OK;
            }

            // Find the longest child SceHwSource
            for (uint32_t i = 0; i < getSourceCount(); i++)
            {
                SourceInternal* pSource = getChildSource(i);

                if (pSource != NULL)
                {
                    if (pSource->loopCount() == knLoopInfinite)
                    {
                        *pnDurationOut = knDurationInfinite;
                        return SCE_HEATWAVE_API_OK;
                    }
                    else if (pSource->duration() == knDurationUnknown)
                    {
                        *pnDurationOut = knDurationUnknown;
                        return SCE_HEATWAVE_API_OK;
                    }

                    // Duration of the SceHwSource must take the loop count into account.
                    nCurrentSourceDuration = pSource->duration() * (pSource->loopCount() + 1);
                    if (nCurrentSourceDuration > nMaxSourceDuration)
                    {
                        nMaxSourceDuration = nCurrentSourceDuration;
                    }
                }
            }

            // Duration is the longest SceHwSource multiplied by the SceHwSound loop count.
            *pnDurationOut = nMaxSourceDuration * (m_nLoopCount + 1);
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Return the contents of m_sourceMap[uIndex]
        * @note Could be NULL (empty slot)
        */
        SourceInternal* SoundInternal::getChildSource(uint32_t uIndex) const
        {
            if (uIndex >= _sceHeatWaveGetMaxSourceCount())
            {
                return NULL;
            }
            return m_sourceMap[uIndex];
        }

        /**
        * @brief Return the contents of m_sfxMap[uIndex]
        * @note Could be NULL (empty slot)
        */
        SfxInternal* SoundInternal::getSfxHandle(uint32_t uIndex)
        {
            if (uIndex >= _sceHeatWaveGetMaxSfxCount())
            {
                return NULL;
            }
            return m_sfxMap[uIndex];
        }

        uint32_t SoundInternal::getIndex() const
        {
            return m_index;
        }

    } /* HeatWave */
} /* sce */
