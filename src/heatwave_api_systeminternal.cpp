/* SIE CONFIDENTIAL
 * HeatWave 1.5
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include "heatwave_api_systeminternal.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_handleconvert.h"
#include "heatwave_api_platform.h"
#include "heatwave_api_panner.h"
#include "heatwave_api_errors.h"
#include "heatwave_api_synthinterface.h"
#include <math.h>

/* When compiling with VS2012 compiler, roundf is not defined. */
#if defined(_MSC_VER)
#if (_MSC_VER <= 1700)
static float roundf(float x)
{
    return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}
#endif /* _MSC_VER <= 1700 */
#endif /* _MSC_VER */

namespace sce{
    namespace HeatWave{
        #define SCE_HEATWAVE_UPDATE_SLEEP_MS 5

        static SystemInternal *s_globalSystemHandle = NULL; /* One global System for now (singleton), can add support for multiple systems later if needed */

        /**
        * @brief update the system (clean up any resources that are finished), trigger streaming of audio data
        * This runs on an internal thread
        */
        int32_t UpdateThread(void *pUserData)
        {
            SystemInternal *systemHandle = (SystemInternal*)pUserData;
            if (systemHandle == NULL)
            {
                return SCE_HEATWAVE_THREAD_FAILURE;
            }

            while (systemHandle->isUpdateThreadRunning()) /* Make sure we have not been flagged to stop processing */
            {
                systemHandle->update();
                _sceHeatWaveSleep(SCE_HEATWAVE_UPDATE_SLEEP_MS);
            }
            return SCE_HEATWAVE_THREAD_SUCCESS;
        }

        /**
        * @brief Create the system singleton, if it already exists do not replace
        * @return The system singleton
        */
        SystemInternal* SystemInternal::createInstance()
        {
            if (!getInstance())
            {
                s_globalSystemHandle = new SystemInternal();
            }
            return s_globalSystemHandle;
        }

        /**
        * @brief System is a singleton.
        * @return The existing singleton instance
        */
        SystemInternal* SystemInternal::getInstance()
        {
            return s_globalSystemHandle;
        }

        SystemInternal::SystemInternal()
            : m_soundMap(NULL),
            m_soundCount(0),
            m_globalMutex(NULL),
            m_systemUpdateThread(NULL),
            m_isInitialised(false),
            m_synth(NULL),
            m_callbackFunc(SCE_HEATWAVE_API_NO_CALLBACK),
            m_callbackUserData(NULL)
        {
            m_panner = new Panner();

            m_callbackInfo.uCurrentA3dObjectCount = 0;
            m_callbackInfo.uPreviousA3dObjectCount = 0;
        }

        SystemInternal::~SystemInternal()
        {
            s_globalSystemHandle = NULL;

            if (m_globalMutex != NULL)
            {
                delete m_globalMutex;
                m_globalMutex = NULL;
            }

            if (m_panner != NULL)
            {
                delete m_panner;
                m_panner = NULL;
            }
        }

        /**
        * @brief Has the system been initialised - has ::init() already been called?
        */
        bool SystemInternal::isInitialised()
        {
            return m_isInitialised;
        }

        /**
        * @brief Setup worker thread, global mutex and allocate memory
        * @return false if any of above setup fails
        * @param pSynth The platform specific synth layer to use
        */
        bool SystemInternal::init(PlatformSynth *pSynth)
        {
            SceHwError ret = SCE_HEATWAVE_API_OK;
            bool inLoopbackMode = false;

            if (pSynth == NULL)
            {
                return false;
            }

            m_synth = pSynth;

            m_globalMutex = _sceHeatWaveCreateMutex();
            if (m_globalMutex == NULL)
            {
                return false;
            }

            m_soundMap = (SoundInternal**)MemoryHandler::mallocCb(sizeof(SoundInternal*) * _sceHeatWaveGetMaxSoundCount());
            if (m_soundMap == NULL)
            {
                return false;
            }
            MemoryHandler::memsetCb(m_soundMap, 0, sizeof(SoundInternal*) * _sceHeatWaveGetMaxSoundCount());

            ret = pSynth->isLoopback(&inLoopbackMode);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return false;
            }

            if (!inLoopbackMode)
            {
                m_systemUpdateThread = _sceHeatWaveCreateThread();
                if (m_systemUpdateThread == NULL)
                {
                    return false;
                }
                m_systemUpdateThread->start(UpdateThread, this);
            }

            m_isInitialised = true;
            return true;
        }

        /**
        * @brief Clean up resources that we created in ::init()
        */
        void SystemInternal::release()
        {
            Thread *pSystemUpdateThread = m_systemUpdateThread;

            m_isInitialised = false;

            if (pSystemUpdateThread != NULL)
            {
                m_systemUpdateThread->stop();
                m_systemUpdateThread = NULL;

                delete pSystemUpdateThread;
            }

            if (m_soundMap != NULL)
            {
                SoundInternal *current = NULL;
                for (uint32_t i = 0; i < _sceHeatWaveGetMaxSoundCount(); i++)
                {
                    current = m_soundMap[i];
                    if (current != NULL)
                    {
                        delete current;
                        m_soundMap[i] = 0;
                    }
                }

                MemoryHandler::freeCb(m_soundMap);
                m_soundMap = NULL;
            }

            m_soundCount = 0;
            m_activeSoundList.clear();
        }

        /**
        * @brief update all the SoundInternal.  Internally they will 
        * flush the update() to children
        *
        * System::update ---> Sound::update ----> Sfx::update ----> Voice::update
        *              |                  |                 |-----> Voice::update
        *              |                  |
        *              |                  |----> Sfx::update
        *              |
        *              |----> Sound::update
        */
        void SystemInternal::update()
        {
            SCE_HEATWAVE_LOCK(); /* lock the global mutex, this will stop user land api functions been able to get in whilst update is processing */

            SoundInternal *current = NULL;

            for (std::list<SoundInternal*>::iterator it = m_activeSoundList.begin(); it != m_activeSoundList.end(); it++)
            {
                current = *it;
                current->update();
            }

            processCallback();

            SCE_HEATWAVE_UNLOCK();
        }

        /**
        * @brief Update the parameters of all SoundInternal.
        *
        * Recalculate the internal parameters of all SoundInternal and their 
        * children (for instance if the listener position changes).
        */
        SceHwError SystemInternal::updateParameters()
        {
            SceHwError res = SCE_HEATWAVE_API_OK;
            SoundInternal *current = NULL;

            for (std::list<SoundInternal*>::iterator it = m_activeSoundList.begin(); it != m_activeSoundList.end(); it++)
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

        /**
        * @brief Find the first available slot in m_soundMap and create a SoundInternal in it
        */
        bool SystemInternal::createSound(SoundInternal **ppOutSoundHandle)
        {
            int32_t newSoundID = -1;
            SoundInternal *soundHandle = NULL;

            if (ppOutSoundHandle == NULL)
            {
                return false;
            }

            if (m_soundCount >= _sceHeatWaveGetMaxSoundCount()) /* Full */
            {
                return false;
            }

            /* Get an available slot for the new sound */
            for (uint32_t i = 0; i < _sceHeatWaveGetMaxSoundCount(); i++)
            {
                if (m_soundMap[i] == NULL)
                {
                    newSoundID = i;
                    break;
                }
            }

            if (newSoundID == -1) /* No space for any more sounds */
            {
                return false;
            }

            soundHandle = new SoundInternal(newSoundID);
            if (soundHandle == NULL)
            {
                return false;
            }

            if (!soundHandle->initialise())
            {
                delete soundHandle;
                return false;
            }

            m_soundMap[newSoundID] = soundHandle;
            *ppOutSoundHandle     = soundHandle;
            m_activeSoundList.push_back(soundHandle);

            m_soundCount++;

            return true;
        }

        /**
        * @brief Check pSoundHandle belongs to this, and release if it does
        * @note All SoundInternal belong to the system, but allows support for multiple systems...
        */
        bool SystemInternal::releaseSound(SoundInternal *pSoundHandle)
        {
            if (pSoundHandle == NULL)
            {
                return false;
            }

            if (pSoundHandle->getIndex() > _sceHeatWaveGetMaxSoundCount())
            {
                return false;
            }

            if (m_soundMap[pSoundHandle->getIndex()] != pSoundHandle) /* Make sure matches up internally */
            {
                return false;
            }

            m_soundMap[pSoundHandle->getIndex()] = NULL;
            m_activeSoundList.remove(pSoundHandle);
            delete pSoundHandle;

            m_soundCount--;

            return true;
        }

        /**
        * @brief Return the contents of m_soundMap[uIndex]
        * @note Could be NULL; cannot guarentee SoundInternal are created and released in order
        */
        SoundInternal* SystemInternal::getSound(uint32_t uIndex)
        {
            if (uIndex >= _sceHeatWaveGetMaxSoundCount())
            {
                return NULL;
            }
            return m_soundMap[uIndex];
        }

        /**
        * @brief The number of occupied slots in m_soundMap
        */
        int32_t SystemInternal::getSoundCount()
        {
            return m_soundCount;
        }

        void SystemInternal::lock()
        {
            m_globalMutex->lock();
        }

        void SystemInternal::unlock()
        {
            m_globalMutex->unlock();
        }

        /**
        * @brief Check that the worker thread is running and has not been flagged to stop
        */
        bool SystemInternal::isUpdateThreadRunning()
        {
            if (m_systemUpdateThread == NULL)
            {
                return false;
            }

            return m_systemUpdateThread->isRunning();
        }

        /**
        * @brief Get the handle to the synth that HeatWave is using
        */
        PlatformSynth* SystemInternal::getSynth() const
        {
            return m_synth;
        }

        /**
        * @brief Get the handle to the global panner
        */
        Panner* SystemInternal::getPanner()
        {
            return m_panner;
        }

        SceHwError SystemInternal::setCallback(SceHwSystemCallbackFunction pCallbackFunc, void *pUserData /*= NULL*/)
        {
            m_callbackFunc = pCallbackFunc;
            m_callbackUserData = pUserData;

            return SCE_HEATWAVE_API_OK;
        }

        /**
        * @brief Get the current time in milliseconds.
        *
        * Time in milliseconds that HeatWave has been initialised and running for.
        * The time updates are driven by the render of audio data (see PlatformSynth) rather than system time.
        *
        * @param[out] pnTimeOut The current millisecond time.
        *
        * @return See <c>SCE_HEATWAVE_API_OK</c> and other return codes.
        */
        SceHwError SystemInternal::getCurrentTimeMs(int32_t *pnTimeOut) const
        {
            PlatformSynthInfo info;
            SceHwError nRet = SCE_HEATWAVE_API_OK;
            const float kfSecsToMs = 1000.0f;

            if (pnTimeOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }
            if (getSynth() == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            *pnTimeOut = knCurrentTimeUnkown;

            nRet = getSynth()->getInfo(&info);
            if (SCE_HEATWAVE_API_FAILED(nRet))
            {
                return nRet;
            }

            *pnTimeOut = (int32_t)roundf(((float)info.nSamplesElapsed / (float)info.nSampleRate) * kfSecsToMs);
            return SCE_HEATWAVE_API_OK;
        }

        SceHwError SystemInternal::processCallback()
        {
            uint32_t a3dObjectCount = 0;
            SceHwError ret = SCE_HEATWAVE_API_OK;

            if (getSynth() == NULL)
            {
                return SCE_HEATWAVE_API_ERROR;
            }

            ret = getSynth()->getAudio3dObjectCount(&a3dObjectCount);
            if (SCE_HEATWAVE_API_FAILED(ret))
            {
                return ret;
            }

            if (m_callbackInfo.uCurrentA3dObjectCount != a3dObjectCount) /* Fire callback if the number of Audio3d objects has changed */
            {
                m_callbackInfo.uPreviousA3dObjectCount = m_callbackInfo.uCurrentA3dObjectCount;
                m_callbackInfo.uCurrentA3dObjectCount = a3dObjectCount;

                if (m_callbackFunc != SCE_HEATWAVE_API_NO_CALLBACK)
                {
                    m_callbackFunc(&m_callbackInfo, m_callbackUserData, eSysCbType_A3dObjectCount);
                }
            }

            return SCE_HEATWAVE_API_OK;
        }

    } /* HeatWave */
} /* sce */
