/* SIE CONFIDENTIAL
* HeatWave 1.5
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#if defined SCE_HEATWAVE_CODEC_MP3

#include "MP3\heatwave_api_decoder_mp3_win.h"
#include <Propvarutil.h>

#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

namespace sce{
    namespace HeatWave{

        template <class T> void SafeRelease(T **ppT)
        {
            if (*ppT)
            {
                (*ppT)->Release();
                *ppT = NULL;
            }
        }

        DecoderMp3* DecoderMp3::createMp3DecoderInstance()
        {
            return new DecoderMp3Win();
        }

        SceHwError DecoderMp3::loadCodecLibraries(int32_t nMaxChannelNum)
        {
            int hr;
            hr = MFStartup(MF_VERSION);
            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3::unloadCodecLibraries()
        {
            int hr;
            hr = MFShutdown();
            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }
            
            return SCE_HEATWAVE_API_OK;
        }

        bool DecoderMp3::isAvailable(int32_t nChannelNum)
        {
            return true;
        }

        DecoderMp3Win::DecoderMp3Win()
            : DecoderMp3(ePull),
            pDataSourceStream(NULL),
            pReader(NULL),
            pAudioType(NULL)
        {
            nBufferSize = 0;
            nBufferWriteSize = 0;
            pBuffer = NULL;
            nBufferReadPosition = 0;
        }

        DecoderMp3Win::~DecoderMp3Win()
        {
            releaseInternal();
        }

        SceHwError DecoderMp3Win::initInternal()
        {
            int hr;

            pDataSourceStream = new DataSourceByteStream();
            hr = pDataSourceStream->Init(m_dataSource);
            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            hr = MFCreateSourceReaderFromByteStream((IMFByteStream *)pDataSourceStream, NULL, &pReader);
            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            hr = ConfigureAudioStream(pReader, &pAudioType);
            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT;
            }

            return SCE_HEATWAVE_API_OK;
        }

        void DecoderMp3Win::releaseInternal()
        {
            if (pDataSourceStream != NULL)
            {
                delete pDataSourceStream;
                pDataSourceStream = NULL;
            }
        }

        SceHwError DecoderMp3Win::fillInternal(BitStream *pBuffer, uint32_t *puBytesConsumed)
        {
            *puBytesConsumed = 0;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3Win::RenderData()
        {
            int hr;
            IMFSample *pSample = NULL;
            IMFMediaBuffer *pMediaBuffer = NULL;
            DWORD dwFlags = 0;
            BYTE *pAudioData = NULL;
            DWORD cbBuffer = 0;

            // Read the next sample.
            hr = pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);

            if (FAILED(hr))
            {
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                return SCE_HEATWAVE_API_ERROR_FILE;
            }

            if (pSample == NULL)
            {
                return SCE_HEATWAVE_API_OK;
            }

            // Get a pointer to the audio data in the sample.
            hr = pSample->ConvertToContiguousBuffer(&pMediaBuffer);
            if (FAILED(hr)) 
            { 
                pSample->Release();
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            hr = pMediaBuffer->Lock(&pAudioData, NULL, &cbBuffer);
            if (FAILED(hr)) 
            { 
                pMediaBuffer->Release();
                pSample->Release();
                return SCE_HEATWAVE_API_ERROR_INVALID_OPERATION;
            }

            if (cbBuffer > (unsigned int)nBufferSize) //free and realloc
            {
                if (pBuffer != NULL)
                {
                    m_dataSource->freeCb(pBuffer);
                }

                pBuffer = (char *)m_dataSource->mallocCb(cbBuffer);
                nBufferSize = cbBuffer;
            }

            MemoryHandler::memcpyCb(pBuffer, pAudioData, cbBuffer);
            nBufferWriteSize = cbBuffer;
            nBufferReadPosition = 0;

            // Unlock the buffer.
            hr = pMediaBuffer->Unlock();
            pAudioData = NULL;

            SafeRelease(&pMediaBuffer);
            SafeRelease(&pSample);

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3Win::renderInternal(int16_t *pDecodedDataOut, uint32_t uSizeBytes, uint32_t *pNumBytesOut)
        {
            uint32_t uBytesAvailable = nBufferWriteSize - nBufferReadPosition;

            if (pDecodedDataOut == NULL || uSizeBytes == 0 || pNumBytesOut == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            if (uBytesAvailable > uSizeBytes) uBytesAvailable = uSizeBytes;

            if (uBytesAvailable == 0) //emptied the buffer
            {
                SceHwError nRes = RenderData();

                if (nRes == SCE_HEATWAVE_API_ERROR_FILE) //EOF
                {
                    return SCE_HEATWAVE_API_ERROR_FILE;
                }
                else if (nRes != SCE_HEATWAVE_API_OK)
                {
                    return nRes;
                }

                uBytesAvailable = nBufferWriteSize - nBufferReadPosition;
                if (uBytesAvailable > uSizeBytes) uBytesAvailable = uSizeBytes;
            }

            MemoryHandler::memcpyCb(pDecodedDataOut, &pBuffer[nBufferReadPosition], uBytesAvailable);
            *pNumBytesOut = uBytesAvailable;

            nBufferReadPosition += uBytesAvailable;

            return SCE_HEATWAVE_API_OK;
        }

        SceHwError DecoderMp3Win::resetInternal()
        {
            PROPVARIANT var;
            InitPropVariantFromInt64(0, &var);

            pReader->SetCurrentPosition(GUID_NULL, var);

            return SCE_HEATWAVE_API_OK;
        }


        HRESULT DecoderMp3Win::ConfigureAudioStream(IMFSourceReader *pReader, IMFMediaType **ppPCMAudio)
        {
            IMFMediaType *pUncompressedAudioType = NULL;
            IMFMediaType *pPartialType = NULL;

            if (pReader == NULL)
            {
                return E_INVALIDARG;
            }

            // Select the first audio stream, and deselect all other streams.
            HRESULT hr = pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);

            if (SUCCEEDED(hr))
            {
                hr = pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
            }

            // Create a partial media type that specifies uncompressed PCM audio.
            hr = MFCreateMediaType(&pPartialType);

            if (SUCCEEDED(hr))
            {
                hr = pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            }

            if (SUCCEEDED(hr))
            {
                hr = pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
            }

            // Set this type on the source reader. The source reader will
            // load the necessary decoder.
            if (SUCCEEDED(hr))
            {
                hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType);
            }

            // Get the complete uncompressed format.
            if (SUCCEEDED(hr))
            {
                hr = pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType);
            }

            // Ensure the stream is selected.
            if (SUCCEEDED(hr))
            {
                hr = pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
            }

            // Return the PCM format to the caller.
            if (SUCCEEDED(hr))
            {
                *ppPCMAudio = pUncompressedAudioType;
                (*ppPCMAudio)->AddRef();
            }

            SafeRelease(&pUncompressedAudioType);
            SafeRelease(&pPartialType);
            return hr;
        }

    } /* HeatWave */
} /* sce */

#endif /* SCE_HEATWAVE_CODEC_MP3 */