/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

/**
* @file heatwave_api_decoder_mp3_buffer_win.h
*
* @brief IMFByteStream override for accessing Data Sources.
*
* @history
*   1.0 File added.
*   1.3 Change SetCurrentPosition to clamp to the end of the stream & return 
*       S_OK when seeking beyond the end of the stream.
*/

#include <windows.h>
#include <mfapi.h>
#include "heatwave_api_decoder_mp3.h"
#include "../heatwave_api_datasource.h"

#ifndef _SCE_HEATWAVE_API_DECODER_MP3_WIN_BUFFER_H_
#define _SCE_HEATWAVE_API_DECODER_MP3_WIN_BUFFER_H_

namespace sce{
    namespace HeatWave{
        class DataSourceByteStream : IMFByteStream
        {
        public:
            DataSourceByteStream();
            ~DataSourceByteStream();
            int Init(DataSource *pDataSource);

            // Overrides
            STDMETHODIMP QueryInterface(REFIID aIId, LPVOID *aInterface);
            STDMETHODIMP_(ULONG) AddRef();
            STDMETHODIMP_(ULONG) Release();
            HRESULT STDMETHODCALLTYPE GetCapabilities(DWORD *pdwCapabilities);
            HRESULT STDMETHODCALLTYPE GetLength(QWORD *pqwLength);
            HRESULT STDMETHODCALLTYPE SetLength(QWORD qwLength);
            HRESULT STDMETHODCALLTYPE GetCurrentPosition(QWORD *pqwPosition);
            HRESULT STDMETHODCALLTYPE SetCurrentPosition(QWORD qwPosition);
            HRESULT STDMETHODCALLTYPE IsEndOfStream(BOOL *pfEndOfStream);
            HRESULT STDMETHODCALLTYPE Read(BYTE *pb, ULONG cb, ULONG *pcbRead);
            HRESULT STDMETHODCALLTYPE BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
            HRESULT STDMETHODCALLTYPE EndRead(IMFAsyncResult *pResult, ULONG *pcbRead);
            HRESULT STDMETHODCALLTYPE Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten);
            HRESULT STDMETHODCALLTYPE BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
            HRESULT STDMETHODCALLTYPE EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten);
            HRESULT STDMETHODCALLTYPE Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition);
            HRESULT STDMETHODCALLTYPE Flush();
            HRESULT STDMETHODCALLTYPE Close();

        private:
            bool bInit;
            HRESULT DoGetInterface(IUnknown* aUnknown, void** aInterface);
            DataSource* pDataSource;
            ULONG nLastRead;
            int nReference;
        };

    } /* HeatWave */
} /* sce */

#endif /* _SCE_HEATWAVE_API_DECODER_MP3_WIN_BUFFER_H_ */
