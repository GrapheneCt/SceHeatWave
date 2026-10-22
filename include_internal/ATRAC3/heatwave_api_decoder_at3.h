/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_at3.h
*
* @brief Wrapper for ATRAC3 decoding across the various platforms
*
*/

#ifndef _SCE_HEATWAVE_DECODER_AT3_H_
#define _SCE_HEATWAVE_DECODER_AT3_H_

#if defined SCE_HEATWAVE_CODEC_AT3

#include "heatwave_api_errors.h"
#include "heatwave_api_types.h"
#include "..\heatwave_api_memory.h"

namespace sce{
    namespace HeatWave {

        struct BitStream;
        class DataSource;

        class DecoderAt3 : public MemoryHandler
        {
        public:
            typedef enum DecoderAt3Type
            {
                ePushPull = 0, /* feed in blocks of encoded data, render decoded data */
                ePull, /* manages data source internally, render decoded data only */
                eMax
            } DecoderAt3Type;

            typedef enum DecoderAt3CodecType
            {
                eType_AT3 = 0,
                eType_AT3P = 1
            } DecoderAt3CodecType;

            static DecoderAt3* createAt3DecoderInstance(); ///< Factory

            static SceHwError loadCodecLibraries(int32_t nMaxChannelNum);
            static SceHwError unloadCodecLibraries();

            static bool isAvailable(int32_t nChannelNum);

            virtual ~DecoderAt3();

            SceHwError init(DataSource *pSource, DecoderAt3CodecType uCodecType, uint8_t uParam1, uint8_t uParam2);
            void release();

            SceHwError fill(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError getSeekOffset(uint32_t uTime, uint32_t *puOffset);

            SceHwError reset();

            DecoderAt3Type getType() const;

        protected:
            DecoderAt3(DecoderAt3Type eType = ePushPull);

            DecoderAt3CodecType m_codecType;
            uint8_t m_param1;
            uint8_t m_param2;
            DataSource* m_dataSource;
            DecoderAt3Type m_decoderType;

            virtual SceHwError initInternal() =0;
            virtual void releaseInternal() =0;
            virtual SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed) =0;
            virtual SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut) =0;
            virtual SceHwError resetInternal() =0;
            virtual SceHwError getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset) =0;
        };

    } /* sce */
} /* HeatWave */

#endif /* SCE_HEATWAVE_CODEC_AT3 */

#endif /* _SCE_HEATWAVE_DECODER_AT3_H_ */
