/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_at9.h
*
* @brief Wrapper for ATRAC9 decoding across the various platforms
*
*/

#ifndef _SCE_HEATWAVE_DECODER_AT9_H_
#define _SCE_HEATWAVE_DECODER_AT9_H_

#if defined SCE_HEATWAVE_CODEC_AT9

#include "heatwave_api_errors.h"
#include "heatwave_api_types.h"
#include "..\heatwave_api_memory.h"

namespace sce{
    namespace HeatWave {

        struct BitStream;
        class DataSource;

        class DecoderAt9 : public MemoryHandler
        {
        public:
            typedef enum DecoderAt9Type
            {
                ePushPull = 0, /* feed in blocks of encoded data, render decoded data */
                ePull, /* manages data source internally, render decoded data only */
                eMax
            } DecoderAt9Type;

            static const uint32_t kuMaxSamplesPerAt9Frame = 1024; ///< Maximum number of PCM samples per channel an ATRAC9 frame can contain

            static DecoderAt9* createAt9DecoderInstance(); ///< Factory

            static SceHwError loadCodecLibraries(int32_t nMaxChannelNum);
            static SceHwError unloadCodecLibraries();

            static bool isAvailable(int32_t nChannelNum);

            virtual ~DecoderAt9();

            SceHwError init(DataSource *pSource, uint32_t uNumberOfChannels, uint32_t uSampleRate, int32_t nConfigData);
            void release();

            SceHwError fill(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError getSeekOffset(uint32_t uTime, uint32_t *puOffset);

            SceHwError reset();

            DecoderAt9Type getType() const;

        protected:
            DecoderAt9(DecoderAt9Type eType = ePushPull);

            uint32_t m_numberOfChannels;
            uint32_t m_sampleRate;
            int32_t m_configData;
            DataSource* m_dataSource;
            DecoderAt9Type m_decoderType;

            virtual SceHwError initInternal() =0;
            virtual void releaseInternal() =0;
            virtual SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed) =0;
            virtual SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut) =0;
            virtual SceHwError resetInternal() =0;
            virtual SceHwError getSeekOffsetInternal(uint32_t uTime, uint32_t *puOffset) =0;
        };

    } /* sce */
} /* HeatWave */

#endif /* SCE_HEATWAVE_CODEC_AT9 */

#endif /* _SCE_HEATWAVE_DECODER_AT9_H_ */
