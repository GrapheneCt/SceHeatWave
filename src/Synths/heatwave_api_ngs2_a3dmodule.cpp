/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#ifdef SCE_HEATWAVE_API_USE_AUDIO3D

#include "Synths\heatwave_api_ngs2_a3dmodule.h"

#include <sceerror.h>
#include "Synths\heatwave_api_synth_ngs2.h"

namespace sce{
    namespace HeatWave{

        /**
        * @brief Setup the module ready for processing
        */
        int32_t a3dModuleSetup(SceNgs2UserFx2SetupContext *context)
        {
            A3dModuleWork *work = (A3dModuleWork*)context->work;
            if (work == NULL)
            {
                return SCE_NGS2_ERROR_FAIL;
            }

            work->objectId = SCE_AUDIO3D_OBJECT_INVALID;

            return SCE_OK;
        }

        /**
        * @brief Allocate\deallocate libAudio3d objects, update ilbAudio3d object parameters and pass PCM data to the libAudio3d object
        */
        int32_t a3dModuleProcess(SceNgs2UserFx2ProcessContext *context)
        {
            uint32_t numAttributes = 0;
            SceAudio3dAttribute aAttribute[5];
            Ngs2Synth *synth = NULL;
            int32_t result = SCE_OK;
            const A3dModuleParam *param = NULL;
            A3dModuleWork *work = NULL;
            SceAudio3dPcm pcm;
            SceAudio3dPosition pos;

            if (context == NULL)
            {
                return SCE_NGS2_ERROR_FAIL;
            }

            synth = ((Ngs2Synth*)(context->userData));
            param = (A3dModuleParam*)context->param;
            work = (A3dModuleWork*)context->work;

            if (synth == NULL || param == NULL || work == NULL)
            {
                return SCE_NGS2_ERROR_FAIL;
            }

            /* Reserve a libAudio3d object */
            if (context->flags & SCE_NGS2_USER_FX_FLAG_RESET)
            {
                if (work->objectId == SCE_AUDIO3D_OBJECT_INVALID)
                {
                    result = sceAudio3dObjectReserve(synth->getOutputPortId(), &work->objectId);
                    if (result != SCE_OK)
                    {
                        return result;
                    }

                    synth->IncAudio3dObjectCount();
                }
            }

            /* Update attributes */
            if (context->flags & SCE_NGS2_USER_FX_FLAG_DIRTY)
            {
                pos.fX = param->vPosition.fX;
                pos.fY = param->vPosition.fY;
                pos.fZ = -param->vPosition.fZ; ///< Switch from HeatWave right hand coordinates to libAudio3d left hand

                aAttribute[numAttributes].uiAttributeId = SCE_AUDIO3D_ATTRIBUTE_POSITION;
                aAttribute[numAttributes].pValue = &pos;
                aAttribute[numAttributes].szValue = sizeof(SceAudio3dPosition);
                numAttributes++;

                aAttribute[numAttributes].uiAttributeId = SCE_AUDIO3D_ATTRIBUTE_GAIN;
                aAttribute[numAttributes].pValue = &param->fGain;
                aAttribute[numAttributes].szValue = sizeof(float);
                numAttributes++;

                aAttribute[numAttributes].uiAttributeId = SCE_AUDIO3D_ATTRIBUTE_SPREAD;
                aAttribute[numAttributes].pValue = &param->fSpread;
                aAttribute[numAttributes].szValue = sizeof(float);
                numAttributes++;

                aAttribute[numAttributes].uiAttributeId = SCE_AUDIO3D_ATTRIBUTE_PRIORITY;
                aAttribute[numAttributes].pValue = &param->nPriority;
                aAttribute[numAttributes].szValue = sizeof(int);
                numAttributes++;
            }

            /* Write pcm data */
            {
                pcm.eFormat = SCE_AUDIO3D_FORMAT_FLOAT;
                pcm.pSampleBuffer = context->aChannelData[0];
                pcm.uiNumSamples = context->numGrainSamples;
                aAttribute[numAttributes].uiAttributeId = SCE_AUDIO3D_ATTRIBUTE_PCM;
                aAttribute[numAttributes].pValue = &pcm;
                aAttribute[numAttributes].szValue = sizeof(pcm);

                numAttributes++;
            }

            /* Set Audio 3D object attributes */
            result = sceAudio3dObjectSetAttributes(synth->getOutputPortId(), work->objectId, numAttributes, aAttribute);
            if (result < SCE_OK)
            {
                return result;
            }

            /* Cleanup */
            if (context->flags & SCE_NGS2_USER_FX_FLAG_KILLED)
            {
                if (work->objectId != SCE_AUDIO3D_OBJECT_INVALID)
                {
                    result = sceAudio3dObjectUnreserve(synth->getOutputPortId(), work->objectId);
                    if (result < SCE_OK)
                    {
                        return result;
                    }

                    work->objectId = SCE_AUDIO3D_OBJECT_INVALID;
                    synth->DecAudio3dObjectCount();
                }
            }

            return SCE_OK;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_API_USE_AUDIO3D */
