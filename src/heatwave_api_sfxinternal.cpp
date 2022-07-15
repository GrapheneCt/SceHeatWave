/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_sfxinternal.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_voiceinternal.h"
#include <math.h>
#include "heatwave_api_errors.h"
#include "heatwave_api_systeminternal.h"

namespace sce{
    namespace HeatWave{

        uint32_t SfxInternal::s_globalSfxCount = 0;
        SceHwSfxCallbackFunction SfxInternal::m_callbackFunc = NULL;
        void* SfxInternal::m_callbackUserdata = NULL;

        /**
        * @brief Default Sfx playback parameters.
        *
        * The default parameters used if no settings are specified by the user when calling play.
        */
        SceHwSfxPlayParam SfxInternal::s_defaultPlaybackParam = { 
            false,                          ///< Not a libAudio3d object.
            knSfxAudio3dPriorityMin,        ///< Lowest libAudio3d object priority.
            SCE_HEATWAVE_API_OUTPUT_MAIN    ///< Output to the main output.
        };

        SfxInternal::SfxInternal(uint32_t uIndex, SoundInternal *pParent)
            : m_parent(pParent),
            m_index(uIndex),
            m_volume(1.0f),
            m_pitchMultiplier(1.0f),
            m_voices(NULL),
            m_nLoopsRemaining(0),
            m_bIsInitialised(false),
            m_nUpdateTime(0),
            m_nElapsedTime(0),
            m_nSeekOffset(0),
            m_state(kSfxState_Ready)
        {
            m_position.fX = 0.0f;
            m_position.fY = 0.0f;
            m_position.fZ = 0.0f;

            m_velocity.fX = 0.0f;
            m_velocity.fY = 0.0f;
            m_velocity.fZ = 0.0f;

            m_forward.fX =  0.0f;
            m_forward.fY =  0.0f;
            m_forward.fZ = -1.0f;

            m_id = _sceHeatWaveGenerateId();
            SfxInternal::s_globalSfxCount++;

            m_callbackInfo.hSfxHandle = _sceHeatWaveGetSfxHandle(this);
            m_callbackInfo.currentState = kSfxState_Ready;
            m_callbackInfo.previousState = kSfxState_Ready;
        }

        SfxInternal::~SfxInternal()
        {
            SfxInternal::s_globalSfxCount--;
            stop();
            release();
        }

        /**
        * @brief    Setup ready to begin playback.
        *
        *           Set up the voices and initialise them ready to begin playback.
        *
        * @param[in]    pParam  Parameters for playback.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::initialise(SceHwSfxPlayParam *pParam)
        {
            SoundInternal   *pParentSound = getParentSound();
            SourceInternal  *pSource = NULL;
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            if (pParam == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (m_bIsInitialised)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            if (pParentSound == NULL) /* Should not be possible */
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            nRet = m_parent->getLoopCount(&m_nLoopsRemaining);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            /* Loop through the parent sound's sources and create a voice for each source */
            for (uint32_t i = 0; i < _sceHeatWaveGetMaxSourceCount(); i++)
            {
                VoiceInternal *pNewVoice = NULL;

                pSource = pParentSound->getChildSource(i);
                if (pSource == NULL)
                {
                    continue;
                }

                pNewVoice = new VoiceInternal(this);
                if (pNewVoice == NULL)
                {
                    return SCE_HEATWAVE_API_ERROR;
                }
                if (!pNewVoice->initialise(pSource, pParam))
                {
                    delete pNewVoice;
                    return SCE_HEATWAVE_API_ERROR;
                }

                addChildVoice(pNewVoice);
            }

            nRet = SystemInternal::getInstance()->getCurrentTimeMs(&m_nUpdateTime);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                if (nRet != SCE_HEATWAVE_API_ERROR_INVALID_OPERATION)
                {
                    return nRet;
                }
            }

            m_nElapsedTime = 0;
            m_bIsInitialised = true;

            // If a seek was previously called prior to play, actually seek now.
            if (m_nSeekOffset > 0)
            {
                seek(m_nSeekOffset);
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief    Clean-up allocated resources.
        *
        *           Releases all allocated resources and return to an uninitialised state.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::release()
        {
            ChildVoice *pCurrent = m_voices;
            ChildVoice *pNext = NULL;
            bool bSuccess = true;

            if (!m_bIsInitialised)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            while (pCurrent != NULL)
            {
                bSuccess &= pCurrent->voice->stop();

                pNext = pCurrent->next;

                delete pCurrent->voice;
                delete pCurrent;

                pCurrent = pNext;
            }

            m_voices = NULL;
            m_nLoopsRemaining = 0;
            m_bIsInitialised = false;
            m_nElapsedTime = 0;
            m_nSeekOffset = 0;

            if (!bSuccess)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief    Trigger playback of the voices.
        *
        *           Commence playback of the voices.  ::initialise must be called prior to ::play.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::play()
        {
            ChildVoice *pCurrent = m_voices;
            SceHwError nRet = SCE_HEATWAVE_API_OK;

            if (!m_bIsInitialised)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            while (pCurrent != NULL)
            {
                if (!pCurrent->voice->play())
                {
                    nRet = SCE_HEATWAVE_API_ERROR;
                }
                pCurrent = pCurrent->next;
            }

            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            m_state = kSfxState_Playing;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief    Cease playback of the child voices.
        *
        *           Iterate over all the voices and stop processing.
        *
        * @return   See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::stop()
        {
            ChildVoice *pCurrent = m_voices;
            bool bSuccess = true;

            if (m_voices == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            while (pCurrent != NULL)
            {
                bSuccess &= pCurrent->voice->stop();
                pCurrent = pCurrent->next;
            }

            if (!bSuccess)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            m_nElapsedTime = 0;
            m_nSeekOffset = 0;

            m_state = kSfxState_Ready;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Loop through child voices & pause them
        * @return false if any of the voices failed internally
        */
        bool SfxInternal::pause()
        {
            const int32_t knPreviousUpdateTime = m_nUpdateTime;
            bool success = true;
            ChildVoice *current = m_voices;

            if (m_voices == NULL)
            {
                return true; /* No child sources, so nothing to pause, not considered an error! */
            }

            while (current != NULL)
            {
                success &= current->voice->pause();

                current = current->next;
            }

            SystemInternal::getInstance()->getCurrentTimeMs(&m_nUpdateTime);
            m_nElapsedTime += (m_nUpdateTime - knPreviousUpdateTime);

            m_state = kSfxState_Paused;

            return success;
        }

        /**
        * @brief Loop through child voices and resume
        * @return false if any of the voices failed internally
        */
        bool SfxInternal::resume()
        {
            bool success = true;
            SceHwSfxState state = kSfxState_Ready;

            getState(&state);

            if (state == kSfxState_Paused)
            {
                ChildVoice *current = m_voices;

                while (current != NULL)
                {
                    success &= current->voice->resume();

                    current = current->next;
                }
            }

            SystemInternal::getInstance()->getCurrentTimeMs(&m_nUpdateTime);

            m_state = kSfxState_Playing;

            return success;
        }

        /**
        * @brief Add pVoice as a child of this (this keeps a linked list of children)
        */
        void SfxInternal::addChildVoice(VoiceInternal *pVoice)
        {
            ChildVoice *newChild = NULL;

            if (pVoice == NULL)
            {
                return;
            }

            newChild = new ChildVoice();
            if (newChild == NULL)
            {
                return;
            }

            newChild->voice = pVoice;
            newChild->voice->getType(&newChild->voiceType);

            newChild->next  = m_voices; /* Insert as head node */
            m_voices        = newChild;
        }

        /**
        * @brief Get the state of the sfx.
        *
        * Query the current playback state.
        *
        * @param[out] pState Destination for the current state.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::getState(SceHwSfxState *pState) const
        {
            if (pState == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            *pState = m_state;
            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief    Update the childvoices and internal state.
        *
        *           This is called from the internal update thread.
        *           Performs update of the ChildVoices to process data.
        *           If ChildVoices have completed, looping is performed as needed.
        *           Performs clean up on completion.
        */
        void SfxInternal::update()
        {
            ChildVoice *pCurrent = m_voices;
            int32_t     nActiveVoiceCount = 0;
            SceHwSfxState state = kSfxState_Ready;
            const int32_t knPreviousUpdateTime = m_nUpdateTime;

            while (pCurrent != NULL)
            {
                pCurrent->voice->update(); /** Voice processing */

                pCurrent->voice->getState(&state);
                if (state != kSfxState_Ready)
                {
                    nActiveVoiceCount++;
                }

                pCurrent = pCurrent->next;
            }

            if (nActiveVoiceCount == 0) /** All the child voices have completed, either loop or clean up resources */
            {
                if (m_nLoopsRemaining != 0)
                {
                    pCurrent = m_voices;
                    while (pCurrent != NULL)
                    {
                        pCurrent->voice->reset();
                        pCurrent = pCurrent->next;
                    }

                    play();

                    if (m_nLoopsRemaining != knLoopInfinite)
                    {
                        --m_nLoopsRemaining;
                    }
                }
                else
                {
                    m_state = kSfxState_Ready;
                    release();
                }
            }

            SystemInternal::getInstance()->getCurrentTimeMs(&m_nUpdateTime);

            getState(&state);
            if (state == kSfxState_Playing)
            {
                m_nElapsedTime += (m_nUpdateTime - knPreviousUpdateTime);
            }


            processCallback();
        }

        SceHwError SfxInternal::updateParameters()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            ChildVoice *current = m_voices;

            while (current != NULL)
            {
                res = current->voice->updateParameters();
                if (SCE_HEATWAVE_API_FAILED(res))
                {
                    return res;
                }

                current = current->next;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Cache position (so that it can be used when we call play) and flush through to playing child voices
        * @return false if any of the voices failed internally
        */
        bool SfxInternal::setPosition(SceHwVector vPosition)
        {
            bool success = true;
            ChildVoice *current = m_voices;

            m_position = vPosition;

            while (current != NULL)
            {
                if (current->voiceType == eSource3d)
                {
                    success &= SCE_HEATWAVE_API_SUCCEEDED(current->voice->setPosition(m_position));
                }

                current = current->next;
            }

            return success;
        }

        /**
        * @brief Cache velocity (so that it can be used when we call play) and flush through to playing child voices
        * @return false if any of the voices failed internally
        */
        bool SfxInternal::setVelocity(SceHwVector vVelocity)
        {
            bool success = true;
            ChildVoice *current = m_voices;

            m_velocity = vVelocity;

            while (current != NULL)
            {
                if (current->voiceType == eSource3d)
                {
                    success &= SCE_HEATWAVE_API_SUCCEEDED(current->voice->setVelocity(m_velocity));
                }
                current = current->next;
            }

            return success;
        }

        /**
        * @brief Normalise and cache forward (so that it can be used when we call play) and flush through to playing child voices
        * @return false if any of the voices failed internally
        */
        bool SfxInternal::setForward(SceHwVector vForward)
        {
            bool success = true;
            ChildVoice *current = m_voices;

            const float forwardnorm = sqrtf((vForward.fX * vForward.fX) + (vForward.fY * vForward.fY) + (vForward.fZ * vForward.fZ));

            m_forward.fX = vForward.fX / forwardnorm;
            m_forward.fY = vForward.fY / forwardnorm;
            m_forward.fZ = vForward.fZ / forwardnorm;

            while (current != NULL)
            {
                if (current->voiceType == eSource3d)
                {
                    success &= SCE_HEATWAVE_API_SUCCEEDED(current->voice->setForward(m_forward));
                }
                current = current->next;
            }

            return success;
        }

        void SfxInternal::getPosition(SceHwVector *pPositionOut)
        {
            if (pPositionOut == NULL)
            {
                return;
            }

            pPositionOut->fX = m_position.fX;
            pPositionOut->fY = m_position.fY;
            pPositionOut->fZ = m_position.fZ;
        }

        void SfxInternal::getVeloctiy(SceHwVector *pVelocityOut)
        {
            if (pVelocityOut == NULL)
            {
                return;
            }

            pVelocityOut->fX = m_velocity.fX;
            pVelocityOut->fY = m_velocity.fY;
            pVelocityOut->fZ = m_velocity.fZ;
        }

        void SfxInternal::getForward(SceHwVector *pForwardOut)
        {
            if (pForwardOut == NULL)
            {
                return;
            }

            pForwardOut->fX = m_forward.fX;
            pForwardOut->fY = m_forward.fY;
            pForwardOut->fZ = m_forward.fZ;
        }

        /**
        * @brief Cache volume (so that it can be used when we call play) and flush through to playing child voices
        */
        bool SfxInternal::setVolume(float32_t fVol)
        {
            bool success = true;
            ChildVoice *current = m_voices;

            m_volume = fVol;

            while (current != NULL)
            {
                success &= SCE_HEATWAVE_API_SUCCEEDED(current->voice->setVolume(fVol));
                current = current->next;
            }

            return success;
        }

        /**
        * @brief Cache pitch (so that it can be used when we call play) and flush through to playing child voices
        */
        bool SfxInternal::setPitch(float32_t fPitchMul)
        {
            bool success = true;
            ChildVoice *current = m_voices;

            m_pitchMultiplier = fPitchMul;

            while (current != NULL)
            {
                success &= SCE_HEATWAVE_API_SUCCEEDED(current->voice->setPitch(fPitchMul));
                current = current->next;
            }

            return success;
        }

        uint32_t SfxInternal::getIndex() const
        {
            return m_index;
        }

        SoundInternal* SfxInternal::getParentSound()
        {
            return m_parent;
        }

        float32_t SfxInternal::getVolume() const
        {
            return m_volume;
        }

        float32_t SfxInternal::getPitch() const
        {
            return m_pitchMultiplier;
        }

        SceHwError SfxInternal::setCallback(SceHwSfxCallbackFunction pCallbackFunc, void *pUserData)
        {
            m_callbackFunc = pCallbackFunc;
            m_callbackUserdata = pUserData;

            return SCE_HEATWAVE_API_OK;
        }

        int16_t SfxInternal::id() const
        {
            return m_id;
        }

        /**
        * @brief Query the play time elapsed.
        *
        * Return the current elapsed duration of playback in milliseconds .
        *
        * @return Elapsed play time in milliseconds.
        */
        int32_t SfxInternal::getElapsedTime() const
        {
            int32_t nCurrentTime = 0;
            int32_t nDelta = 0;
            SceHwSfxState state = kSfxState_Ready;

            getState(&state);

            if (state == kSfxState_Playing)
            {   // account for any time that has elapsed since the last call to ::update()
                SystemInternal::getInstance()->getCurrentTimeMs(&nCurrentTime);
                nDelta = (nCurrentTime - m_nUpdateTime);
            }

            return m_nElapsedTime + nDelta;
        }

        /**
        * @brief Change the current playback time position.
        *
        * @param[in] nTimeMs The new playback position in milli-seconds.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SfxInternal::seek(int32_t nTimeMs)
        {
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            int32_t nDuration = 0;
            int32_t nLoopCount = 0;
            int32_t nLoopsSkipped = 0;
            ChildVoice *pCurrent = m_voices;
            uint32_t uNumSources = 0;
            int32_t nMaxSourceDuration = 0;

            if (nTimeMs < 0)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            // Don't allow seek beyond end of duration.
            nRet = m_parent->getDuration(&nDuration);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }
            if (nDuration == knDurationUnknown)
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }
            if (nDuration < nTimeMs && nDuration != knDurationInfinite)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            // If initialised and playing, skip to the new playback time.
            // Otherwise, cache so that when ::play() is called playback will start from nTimeMs.
            if (m_bIsInitialised)
            {
                // Calculate the duration of 1 loop.
                uNumSources = m_parent->getSourceCount();

                // Find the longest child voice == the duration 1 loop
                for (uint32_t i = 0; i < uNumSources; i++)
                {
                    SourceInternal *pSource = m_parent->getChildSource(i);
                    int32_t nSourceDuration;

                    if (pSource == NULL)
                    {
                        return SCE_HEATWAVE_API_ERROR;
                    }
                    if (pSource->duration() == knDurationUnknown)
                    {
                        return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
                    }

                    if (pSource->loopCount() == knLoopInfinite)
                    {
                        nMaxSourceDuration = knDurationInfinite;
                        break;
                    }

                    nSourceDuration = pSource->duration() * (pSource->loopCount() + 1);
                    if (nSourceDuration > nMaxSourceDuration) // Duration of a loop is the duration of the longest child source.
                    {
                        nMaxSourceDuration = nSourceDuration;
                    }
                }

                // if the seek will jumps over loops - decrement the remaining loops.
                nRet = m_parent->getLoopCount(&nLoopCount);
                if (SCE_HEATWAVE_API_FAILED(nRet))
                {
                    return nRet;
                }

                if (nLoopCount > 0 && nMaxSourceDuration != knDurationInfinite)
                {
                    nLoopsSkipped = (nTimeMs - (nTimeMs % nMaxSourceDuration)) / nMaxSourceDuration; // number of loops skipped by the seek
                    m_nLoopsRemaining = nLoopCount - nLoopsSkipped;
                }

                // seek the various child sources by the remainder.
                while (pCurrent != NULL)
                {
                    if (nMaxSourceDuration == knDurationInfinite)
                    {
                        nRet = pCurrent->voice->seek(nTimeMs);
                    }
                    else
                    {
                        nRet = pCurrent->voice->seek(nTimeMs % nMaxSourceDuration);
                    }
                    if (nRet == SCE_HEATWAVE_API_ERROR_PARAM)
                    {
                        pCurrent->voice->stop();
                    }
                    else if (SCE_HEATWAVE_API_FAILED(nRet))
                    {
                        return nRet;
                    }

                    pCurrent = pCurrent->next;
                }

                m_nElapsedTime = nTimeMs; // current playback position is now the seek position.
                m_nSeekOffset = 0;
            }
            else // Prepare for playback to start from the specified seek position.
            {
                m_nSeekOffset = nTimeMs;
            }

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Perform callbacks as needed
        */
        SceHwError SfxInternal::processCallback()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            SceHwSfxState state = kSfxState_Ready;

            getState(&state);

            if (m_callbackInfo.currentState != state) /* state has changed, fire the callback */
            {
                m_callbackInfo.previousState = m_callbackInfo.currentState;
                m_callbackInfo.currentState = state;

                if (m_callbackFunc != SCE_HEATWAVE_API_NO_CALLBACK)
                {
                    m_callbackFunc(&m_callbackInfo, m_callbackUserdata, eSfxCbType_StateChange);
                }
            }

            return res;
        }

    } /* HeatWave */
} /* sce */
