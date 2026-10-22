/* SIE CONFIDENTIAL
 * HeatWave 1.6
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_handleconvert.h
*
* @brief Convert between internal and external types
*
* @history
*   1.2 Change handles to reduce possible collisions
*   1.4 Change handles to be a packed uint64_t and increase maximum number of handles.
*/

#ifndef _SCE_HEATWAVE_API_HANDLE_CONVERT_H_
#define _SCE_HEATWAVE_API_HANDLE_CONVERT_H_

#include "heatwave_api.h"

namespace sce{
    namespace HeatWave{

        class SoundInternal;
        class SourceInternal;
        class SfxInternal;

        int16_t _sceHeatWaveGenerateId();

        /**
        * @name Sound
        *
        * @{
        */
        uint32_t        _sceHeatWaveGetMaxSoundCount();                         ///< Maximum available sound handles
        SceHwSound      _sceHeatWaveGetSoundHandle(SoundInternal *pSoundInt);   ///< Internal to external
        SoundInternal*  _sceHeatWaveGetSoundInternal(SceHwSound hSoundHandle);  ///< External to internal
        /** @} */ /* Sound */

        /**
        * @name Source
        *
        * @{
        */
        uint32_t        _sceHeatWaveGetMaxSourceCount();                            ///< Maximum available source handles
        SceHwSource     _sceHeatWaveGetSourceHandle(SourceInternal *pSourceInt);    ///< Internal to external
        SourceInternal* _sceHeatWaveGetSourceInternal(SceHwSource hSourceHandle);   ///< External to internal
        /** @} */ /* Source */

        /**
        * @name Sfx
        *
        * @{
        */
        uint32_t        _sceHeatWaveGetMaxSfxCount();                               ///< Maximum available Sfx handles
        SceHwSfx        _sceHeatWaveGetSfxHandle(SfxInternal *pSfxInt);     ///< Internal to external
        SfxInternal*    _sceHeatWaveGetSfxInternal(SceHwSfx hSfxHandle);    ///< External to internal
        /** @} */ /* Sfx */

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_HANDLE_CONVERT_H_ */
