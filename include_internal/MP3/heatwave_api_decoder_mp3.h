/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3.h
*
* @brief Wrapper for MP3 decoding across the various platforms
*
* @history
*   1.1     File added
*   1.44    Fix ::fill() incorrectly finding MP3 frame sync flags.
*/

#ifndef _SCE_HEATWAVE_DECODER_MP3_H_
#define _SCE_HEATWAVE_DECODER_MP3_H_

#include "heatwave_api_errors.h"
#include "heatwave_api_types.h"
#include "heatwave_api_mp3packetheader.h"
#include "..\heatwave_api_memory.h"

namespace sce{
    namespace HeatWave {

        struct BitStream;
        class DataSource;

        class DecoderMp3 : public MemoryHandler
        {
        public:
            typedef enum DecoderMp3Type
            {
                ePushPull = 0, /* feed in blocks of encoded data, render decoded data */
                ePull, /* manages data source internally, render decoded data only */
                eMax
            } DecoderMp3Type;

            static const uint32_t kuMaxSamplesPerMp3Frame = 1152; ///< Maximum number of PCM samples per channel an MP3 frame can contain

            static DecoderMp3* createMp3DecoderInstance(); ///< Factory

            static SceHwError loadCodecLibraries(int32_t nMaxChannelNum);
            static SceHwError unloadCodecLibraries();

            static bool isAvailable(int32_t nChannelNum);

            virtual ~DecoderMp3();

            SceHwError init(Mp3PacketHeader *pHeader, DataSource *pSource);
            void release();

            SceHwError fill(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError decode(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);

            SceHwError reset();

            DecoderMp3Type getType() const;

        protected:
            DecoderMp3(DecoderMp3Type eType = ePushPull);

            Mp3PacketHeader m_mp3Info;
            DataSource* m_dataSource;
            DecoderMp3Type m_decoderType;

            virtual SceHwError initInternal() =0;
            virtual void releaseInternal() =0;
            virtual SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed) =0;
            virtual SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut) =0;
            virtual SceHwError resetInternal() =0;
        };

    } /* sce */
} /* HeatWave */

#endif /* _SCE_HEATWAVE_DECODER_MP3_H_ */
