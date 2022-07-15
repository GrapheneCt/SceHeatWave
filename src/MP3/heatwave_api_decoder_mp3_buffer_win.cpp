/* SIE CONFIDENTIAL
* HeatWave 1.44
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
* All Rights Reserved.
*/

#include "MP3\heatwave_api_decoder_mp3_buffer_win.h"
#include "heatwave_api_audiostream.h"

#define CHECK_FOR_INIT() if (bInit == false) { return E_NOINTERFACE;}

namespace sce{
    namespace HeatWave{

        DataSourceByteStream::DataSourceByteStream()
        {
            bInit = false;
            nReference = 0;
        }

        DataSourceByteStream::~DataSourceByteStream()
        {

        }

        int DataSourceByteStream::Init(DataSource *pDataSource)
        {
            if (pDataSource == NULL)
            {
                return SCE_HEATWAVE_API_ERROR_PARAM;
            }

            this->pDataSource = pDataSource;
            bInit = true;

            return 0;
        }

        HRESULT DataSourceByteStream::DoGetInterface(IUnknown* aUnknown, void** aInterface)
        {
            if (!aInterface)
            {
                return E_POINTER;
            }

            *aInterface = aUnknown;
            aUnknown->AddRef();
            
            return S_OK;
        }

        STDMETHODIMP DataSourceByteStream::QueryInterface(REFIID aIId, LPVOID *aInterface)
        {
            if (aIId == IID_IMFByteStream) 
            {
                return DoGetInterface(static_cast<IMFByteStream*>(this), aInterface);
            }
            
            if (aIId == IID_IUnknown) 
            {
                return DoGetInterface(static_cast<IMFByteStream*>(this), aInterface);
            }

            *aInterface = nullptr;
            return E_NOINTERFACE;
        }

        STDMETHODIMP_(ULONG) DataSourceByteStream::AddRef()
        {
            nReference++;
            return  nReference;
        }

        STDMETHODIMP_(ULONG) DataSourceByteStream::Release()
        {
            nReference--;
            return  nReference;
        }

        HRESULT DataSourceByteStream::GetCapabilities(DWORD *pdwCapabilities)
        {
            *pdwCapabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;

            return  S_OK;
        }

        HRESULT DataSourceByteStream::GetLength(QWORD *pqwLength)
        {
            int nRes;
            CHECK_FOR_INIT();
            int nCurrentLocation = pDataSource->tell(); //Get current position for restore
            nRes = pDataSource->seek(0, SEEK_END);
            if (nRes == DataSource::knSeekFail)
            {
                return E_FAIL;
            }

            //Get length
            *pqwLength = pDataSource->tell();

            //Reset position
            nRes = pDataSource->seek(nCurrentLocation, SEEK_SET);
            if (nRes == DataSource::knSeekFail)
            {
                return E_FAIL;
            }

            return  S_OK;
        }

        HRESULT DataSourceByteStream::SetLength(QWORD qwLength)
        {
            return  E_NOTIMPL;
        }

        HRESULT DataSourceByteStream::GetCurrentPosition(QWORD *pqwPosition)
        {
            CHECK_FOR_INIT();

            *pqwPosition = pDataSource->tell();

            return  S_OK;
        }

        HRESULT DataSourceByteStream::SetCurrentPosition(QWORD qwPosition)
        {
            int nRes;
            QWORD qwLength;

            CHECK_FOR_INIT();

            GetLength(&qwLength);
            if (qwPosition > qwLength)
            {
                nRes = pDataSource->seek(0, SEEK_END);
            }
            else
            {
                nRes = pDataSource->seek((uint32_t)qwPosition, SEEK_SET);
            }

            if (nRes != 0)
            {
                return E_FAIL;
            }

            return  S_OK;
        }

        HRESULT DataSourceByteStream::IsEndOfStream(BOOL *pfEndOfStream)
        {
            QWORD nPos, nLength;
            HRESULT hr = S_OK;
            CHECK_FOR_INIT();

            if (pfEndOfStream == NULL)
            {
                return E_INVALIDARG;
            }

            hr = GetCurrentPosition(&nPos);
            if (hr != S_OK)
            {
                return hr;
            }

            hr = GetLength(&nLength);
            if (hr != S_OK)
            {
                return hr;
            }

            *pfEndOfStream = (nPos >= nLength);

            return S_OK;
        }

        HRESULT DataSourceByteStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
        {
            int nActualRead;
            CHECK_FOR_INIT();

            if (pb == NULL || pcbRead == NULL)
            {
                return E_INVALIDARG;
            }

            nActualRead = pDataSource->read(pb, 1, cb);
            *pcbRead = nActualRead;

            return  S_OK;
        }

        HRESULT DataSourceByteStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
        {
            HRESULT hr;
            int nRes;
            IMFAsyncResult *pResult;

            CHECK_FOR_INIT();

            if (pb == NULL)
            {
                return E_INVALIDARG;
            }

            nRes = Read(pb, cb, &nLastRead);
            if (nRes != S_OK)
            {
                return nRes;
            }

            hr = MFCreateAsyncResult(NULL, pCallback, punkState, &pResult);
            if (hr != S_OK)
            {
                return hr;
            }

            hr = MFInvokeCallback(pResult);
            if (hr != S_OK)
            {
                return hr;
            }

            return  S_OK;
        }

        HRESULT DataSourceByteStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
        {
            CHECK_FOR_INIT();

            if (pcbRead == NULL)
            {
                return E_INVALIDARG;
            }

            *pcbRead = nLastRead;

            return  S_OK;
        }

        HRESULT DataSourceByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
        {
            CHECK_FOR_INIT();

            if (pqwCurrentPosition == NULL)
            {
                return E_INVALIDARG;
            }

            if (SeekOrigin == msoBegin)
            {
                pDataSource->seek((uint32_t)llSeekOffset, SEEK_SET);
            }
            else if (SeekOrigin == msoCurrent)
            {
                pDataSource->seek((uint32_t)llSeekOffset, SEEK_CUR);
            }
            
            *pqwCurrentPosition = pDataSource->tell();

            return  S_OK;
        }

        HRESULT DataSourceByteStream::Flush()
        {
            return  S_OK;
        }

        HRESULT DataSourceByteStream::Close()
        {
            return  S_OK;
        }

        HRESULT DataSourceByteStream::Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
        {
            return  E_NOTIMPL;
        }

        HRESULT DataSourceByteStream::BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
        {
            return  E_NOTIMPL;
        }

        HRESULT DataSourceByteStream::EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
        {
            return  E_NOTIMPL;
        }
    } /* HeatWave */
} /* sce */
