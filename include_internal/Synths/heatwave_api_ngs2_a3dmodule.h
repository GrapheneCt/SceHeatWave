/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_ngs2_a3dmodule.h
*
* @brief NGS2 user fx to play NGS2 voices as libAudio3d objects
*
* @note SCE_HEATWAVE_API_USE_AUDIO3D must be defined for the module to be compiled
*
* @history
*   1.2 File added.
*   1.4 Fix for out of scope 'pcm' variable passed to sceAudio3dObjectSetAttributes.
*/

#ifndef _SCE_HEATWAVE_API_NGS2_A3D_MODULE_H_
#define _SCE_HEATWAVE_API_NGS2_A3D_MODULE_H_

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D

#include <audio3d.h>
#include <ngs2.h>
#include "heatwave_api_types.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief User fx module work
        */
        typedef struct A3dModuleWork
        {
            SceAudio3dObjectId objectId; ///< libAudio3d object ID
        } A3dModuleWork;

        /**
        * @brief User fx process parameters
        */
        typedef struct A3dModuleParam
        {
            int32_t nPriority; ///< libAudio3d object priority
            SceHwVector vPosition;
            float fSpread;
            float32_t fGain;
        } A3dModuleParam;

        int32_t a3dModuleSetup(SceNgs2UserFx2SetupContext *context);
        int32_t a3dModuleProcess(SceNgs2UserFx2ProcessContext *context);

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */
#endif /* _SCE_HEATWAVE_API_NGS2_A3D_MODULE_H_ */
