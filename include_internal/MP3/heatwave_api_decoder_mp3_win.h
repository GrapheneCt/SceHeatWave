/* SIE CONFIDENTIAL
* HeatWave 1.6
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3_win.h
*
* @brief Win implementation of MP3 decoding
*
* @history
*   1.1 File added
*/

#ifndef _SCE_HEATWAVE_API_DECODER_MP3_WIN_H_
#define _SCE_HEATWAVE_API_DECODER_MP3_WIN_H_

#if defined SCE_HEATWAVE_CODEC_MP3

#include "heatwave_api_decoder_mp3.h"
#include "heatwave_api_decoder_mp3_buffer_win.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace sce{
    namespace HeatWave{

        class DecoderMp3Win : public DecoderMp3
        {
        public:
            DecoderMp3Win();
            ~DecoderMp3Win();

        protected:
            SceHwError initInternal();
            void releaseInternal();
            SceHwError fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed);
            SceHwError renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut);
            SceHwError resetInternal();

        private:
            HRESULT ConfigureAudioStream(IMFSourceReader *pReader, IMFMediaType **ppPCMAudio);
            SceHwError RenderData();

            DataSourceByteStream *pDataSourceStream;
            IMFSourceReader *pReader;
            IMFMediaType *pAudioType;

            int nBufferSize;
            int nBufferWriteSize;
            char *pBuffer;
            int nBufferReadPosition;
            static const int nEOF = -1;
        };

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */

#endif /* _SCE_HEATWAVE_API_DECODER_MP3_WIN_H_ */
