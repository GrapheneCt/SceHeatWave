/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @description
* Handles are a 64-bit field with the following format
*
* NNNN CCCC CCCC CCCC 0000 0000 0000 0000 FFFF FFFF FFFF FFFF XXXX XXXX XXXX XXXX
*
* N = handle type identifier
* C = handle ID
* 0 = Unused
*
* The remaining bits usage differ according to the handle type:
*
* N = 1, Sound
* F = Unused
* X = sound index
*
* N = 2, Source
* F = Parent Sound index
* X = Source index
*
* N = 3, Sfx
* F = Parent Sound index
* X = Sfx index
*/

#include "heatwave_api_handleconvert.h"
#include "heatwave_api_systeminternal.h"
#include "heatwave_api_soundinternal.h"
#include "heatwave_api_sourceinternal.h"
#include "heatwave_api_sfxinternal.h"

namespace sce{
    namespace HeatWave{

        const uint8_t knHandleTypeSound = 1;
        const uint8_t knHandleTypeSource = 2;
        const uint8_t knHandleTypeSfx = 3;

        /* Handle type */
        #define SCE_HEATWAVE_HANDLE_TYPE_BITS   (0xF000000000000000ULL)
        #define SCE_HEATWAVE_HANDLE_TYPE_SHIFT  (60)

        /* Handle ID */
        #define SCE_HEATWAVE_HANDLE_ID_BITS     (0x0FFF000000000000ULL)
        #define SCE_HEATWAVE_HANDLE_ID_SHIFT    (48)

        /* Sound ID */
        #define SCE_HEATWAVE_SOUND_ID_BITS      (0x00000000FFFF0000ULL)
        #define SCE_HEATWAVE_SOUND_ID_SHIFT     (16)

        /* Source ID */
        #define SCE_HEATWAVE_SOURCE_ID_BITS     (0x000000000000FFFFULL)
        #define SCE_HEATWAVE_SOURCE_ID_SHIFT    (0)

        /* Sfx ID */
        #define SCE_HEATWAVE_SFX_ID_BITS        (0x000000000000FFFFULL)
        #define SCE_HEATWAVE_SFX_ID_SHIFT       (0)

        static int16_t snHandleId = 0;

        int16_t _sceHeatWaveGenerateId()
        {
            int16_t nId = snHandleId;

            snHandleId++;
            snHandleId = snHandleId % (SCE_HEATWAVE_HANDLE_ID_BITS >> SCE_HEATWAVE_HANDLE_ID_SHIFT);

            return nId;
        }

        int16_t _getHandleId(const SceHwHandle uHandle)
        {
            return (uHandle & SCE_HEATWAVE_HANDLE_ID_BITS) >> SCE_HEATWAVE_HANDLE_ID_SHIFT;
        }

        int8_t _getHandleType(const SceHwHandle uHandle)
        {
            return (uHandle & SCE_HEATWAVE_HANDLE_TYPE_BITS) >> SCE_HEATWAVE_HANDLE_TYPE_SHIFT;
        }

        /**
        * @name Sound
        *
        * @{
        */
        uint32_t _sceHeatWaveGetMaxSoundCount()
        {
            return SCE_HEATWAVE_SOUND_ID_BITS >> SCE_HEATWAVE_SOUND_ID_SHIFT;
        }

        SceHwSound _sceHeatWaveGetSoundHandle(SoundInternal *pSoundInt)
        {
            SceHwHandle uHandle = 0;

            if (pSoundInt != NULL)
            {
                const SceHwHandle uTypeBits = SceHwHandle(knHandleTypeSound) << SCE_HEATWAVE_HANDLE_TYPE_SHIFT;
                const SceHwHandle uIdBits = SceHwHandle(pSoundInt->id()) << SCE_HEATWAVE_HANDLE_ID_SHIFT;
                const SceHwHandle uSoundBits = SceHwHandle(pSoundInt->getIndex()) << SCE_HEATWAVE_SOUND_ID_SHIFT;

                uHandle = uTypeBits | uIdBits | uSoundBits;
            }

            return uHandle;
        }

        SoundInternal* _sceHeatWaveGetSoundInternal(SceHwSound hSoundHandle)
        {
            SoundInternal* pSound = NULL;
            const int8_t nHandleType = _getHandleType(hSoundHandle);
            const int16_t nHandleId = _getHandleId(hSoundHandle);

            if (hSoundHandle == kuSceHwInvalidHandle)
            {
                return NULL;
            }

            if (nHandleType == knHandleTypeSound)
            {
                const uint32_t uSoundIndex = (hSoundHandle & SCE_HEATWAVE_SOUND_ID_BITS) >> SCE_HEATWAVE_SOUND_ID_SHIFT;
                SystemInternal *systemHandle = SystemInternal::getInstance();

                if (systemHandle != NULL)
                {
                    pSound = systemHandle->getSound(uSoundIndex);
                    if (pSound != NULL && pSound->id() != nHandleId)
                    {
                        pSound = NULL;
                    }
                }
            }

            return pSound;
        }
        /** @} */ /* Sound */

        /**
        * @name Source
        *
        * @{
        */
        uint32_t _sceHeatWaveGetMaxSourceCount()
        {
            return SCE_HEATWAVE_SOURCE_ID_BITS >> SCE_HEATWAVE_SOURCE_ID_SHIFT;
        }

        SceHwSource _sceHeatWaveGetSourceHandle(SourceInternal *pSourceInt)
        {
            SceHwHandle uHandle = 0;

            if (pSourceInt != NULL)
            {
                const SoundInternal *parentSound = pSourceInt->getParentSound();

                if (parentSound != NULL)
                {
                    const SceHwHandle uTypeBits = SceHwHandle(knHandleTypeSource) << SCE_HEATWAVE_HANDLE_TYPE_SHIFT;
                    const SceHwHandle uIdBits = SceHwHandle(pSourceInt->id()) << SCE_HEATWAVE_HANDLE_ID_SHIFT;
                    const SceHwHandle uSoundBits = SceHwHandle(parentSound->getIndex()) << SCE_HEATWAVE_SOUND_ID_SHIFT;
                    const SceHwHandle uSourceBits = SceHwHandle(pSourceInt->getIndex()) << SCE_HEATWAVE_SOURCE_ID_SHIFT;

                    uHandle = uTypeBits | uIdBits | uSoundBits | uSourceBits;
                }
            }

            return uHandle;
        }

        SourceInternal* _sceHeatWaveGetSourceInternal(SceHwSource hSourceHandle)
        {
            SourceInternal *sourceRetHandle = NULL;

            const int8_t nHandleType = _getHandleType(hSourceHandle);
            const int16_t nHandleId = _getHandleId(hSourceHandle);
            const uint32_t uSoundIndex  = (hSourceHandle & SCE_HEATWAVE_SOUND_ID_BITS)  >> SCE_HEATWAVE_SOUND_ID_SHIFT;
            const uint32_t uSourceIndex = (hSourceHandle & SCE_HEATWAVE_SOURCE_ID_BITS) >> SCE_HEATWAVE_SOURCE_ID_SHIFT;

            if (hSourceHandle == kuSceHwInvalidHandle)
            {
                return NULL;
            }

            if (nHandleType == knHandleTypeSource)
            {
                SystemInternal *systemHandle = SystemInternal::getInstance();
                if (systemHandle != NULL)
                {
                    SoundInternal *soundHandle = systemHandle->getSound(uSoundIndex);
                    if (soundHandle != NULL)
                    {
                        sourceRetHandle = soundHandle->getChildSource(uSourceIndex);

                        if (sourceRetHandle != NULL && sourceRetHandle->id() != nHandleId)
                        {
                            sourceRetHandle = NULL;
                        }
                    }
                }
            }

            return sourceRetHandle;
        }
        /** @} */ /* Source */

        /**
        * @name Sfx
        *
        * @{
        */
        uint32_t _sceHeatWaveGetMaxSfxCount()
        {
            return SCE_HEATWAVE_SFX_ID_BITS >> SCE_HEATWAVE_SFX_ID_SHIFT;
        }

        SceHwSfx _sceHeatWaveGetSfxHandle(SfxInternal *pSfxInt)
        {
            SceHwHandle uHandle = 0;

            if (pSfxInt != NULL)
            {
                SoundInternal *parentSound = pSfxInt->getParentSound();

                if (parentSound != NULL)
                {
                    const SceHwHandle uTypeBits = SceHwHandle(knHandleTypeSfx) << SCE_HEATWAVE_HANDLE_TYPE_SHIFT;
                    const SceHwHandle uIdBits = SceHwHandle(pSfxInt->id()) << SCE_HEATWAVE_HANDLE_ID_SHIFT;
                    SceHwHandle uSoundBits = SceHwHandle(parentSound->getIndex()) << SCE_HEATWAVE_SOUND_ID_SHIFT;
                    SceHwHandle uSfxBits = SceHwHandle(pSfxInt->getIndex()) << SCE_HEATWAVE_SFX_ID_SHIFT;

                    uHandle = uTypeBits | uIdBits | uSoundBits | uSfxBits;
                }
            }

            return uHandle;
        }

        SfxInternal* _sceHeatWaveGetSfxInternal(SceHwSfx hSfxHandle)
        {
            SfxInternal *sfxRetHandle = NULL;

            const int8_t nHandleType = _getHandleType(hSfxHandle);
            const int16_t nHandleId = _getHandleId(hSfxHandle);
            uint32_t uSoundIndex  = (hSfxHandle & SCE_HEATWAVE_SOUND_ID_BITS) >> SCE_HEATWAVE_SOUND_ID_SHIFT;
            uint32_t uSfxIndex    = (hSfxHandle & SCE_HEATWAVE_SFX_ID_BITS)   >> SCE_HEATWAVE_SFX_ID_SHIFT;

            if (hSfxHandle == kuSceHwInvalidHandle)
            {
                return NULL;
            }

            if (nHandleType == knHandleTypeSfx)
            {
                SystemInternal *systemHandle = SystemInternal::getInstance();
                if (systemHandle != NULL)
                {
                    SoundInternal *soundHandle   = systemHandle->getSound(uSoundIndex);
                    if (soundHandle != NULL)
                    {
                        sfxRetHandle = soundHandle->getSfxHandle(uSfxIndex);

                        if (sfxRetHandle != NULL && sfxRetHandle->id() != nHandleId)
                        {
                            sfxRetHandle = NULL;
                        }
                    }
                }
            }

            return sfxRetHandle;
        }
        /** @} */ /* Sfx */

    } /* HeatWave */
} /* sce */
