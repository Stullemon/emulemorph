//this file is part of eMule
//Copyright (C)2003 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "StdAfx.h"
#include "framegrabthread.h"
#include "CxImage/xImage.h"
#include "otherfunctions.h"
#include "emule.h"
#include <Windows.h>
#include "quantize.h"
// DirectShow MediaDet
#include <strmif.h>
#include <uuids.h>
#include <qedit.h>
typedef struct tagVIDEOINFOHEADER {

    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

    BITMAPINFOHEADER bmiHeader;

} VIDEOINFOHEADER;

IMPLEMENT_DYNCREATE(CFrameGrabThread, CWinThread)

CFrameGrabThread::CFrameGrabThread()
{
}

CFrameGrabThread::~CFrameGrabThread()
{
}

BOOL CFrameGrabThread::Run(){
	DbgSetThreadName("FrameGrabThread");
	imgResults = new CxImage*[nFramesToGrab];
	FrameGrabResult_Struct* result = new FrameGrabResult_Struct;
	result->nImagesGrabbed = GrabFrames();
	result->imgResults = imgResults;
	result->pSender = pSender;
	VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FRAMEGRABFINISHED, (WPARAM)pOwner,(LPARAM)result) );
	AfxEndThread(0,true);
	return 0;
}

uint8 CFrameGrabThread::GrabFrames(){
	#define TIMEBETWEENFRAMES	50.0 // could be a param later, if needed
	for (int i = 0; i!= nFramesToGrab; i++)
		imgResults[i] = NULL;
	try{
		HRESULT hr;
		CComPtr<IMediaDet> pDet;
		CoInitialize(NULL);
		hr = pDet.CoCreateInstance(__uuidof(MediaDet));
		if (!SUCCEEDED(hr))
			return 0;

		// Convert the file name to a BSTR.
		CComBSTR bstrFilename(strFileName);
		hr = pDet->put_Filename(bstrFilename);

		long lStreams;
		bool bFound = false;
		hr = pDet->get_OutputStreams(&lStreams);
		for (long i = 0; i < lStreams; i++)
		{
			GUID major_type;
			hr = pDet->put_CurrentStream(i);
			hr = pDet->get_StreamType(&major_type);
			if (major_type == MEDIATYPE_Video)
			{
				bFound = true;
				break;
			}
		}
		
		if (!bFound)
			return 0;
		
		double dLength = 0;
		pDet->get_StreamLength(&dLength);
		if (dStartTime > dLength)
			dStartTime = 0;

		long width = 0, height = 0; 
		AM_MEDIA_TYPE mt;
		hr = pDet->get_StreamMediaType(&mt);
		if (mt.formattype == FORMAT_VideoInfo) 
		{
			VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)(mt.pbFormat);
			width = pVih->bmiHeader.biWidth;
			height = pVih->bmiHeader.biHeight;
			
	        
			// We want the absolute height, don't care about orientation.
			if (height < 0) height *= -1;
		}
		else {
			return 0; // Should not happen, in theory.
		}

		/*FreeMediaType(mt); = */	
		if (mt.cbFormat != 0){
			CoTaskMemFree((PVOID)mt.pbFormat);
			mt.cbFormat = 0;
			mt.pbFormat = NULL;
		}
		if (mt.pUnk != NULL){
			mt.pUnk->Release();
			mt.pUnk = NULL;
		}
		/**/

	    
		long size;
		uint32 nFramesGrabbed;
		for (nFramesGrabbed = 0; nFramesGrabbed != nFramesToGrab; nFramesGrabbed++){
			hr = pDet->GetBitmapBits(dStartTime + (nFramesGrabbed*TIMEBETWEENFRAMES), &size, NULL, width, height);
			if (SUCCEEDED(hr)) 
			{
				// we could also directly create a Bitmap in memory, however this caused problems/failed with *some* movie files
				// when I tried it for the MMPreview, while this method works always - so I'll continue to use this one
				long nFullBufferLen = sizeof( BITMAPFILEHEADER ) + size;
				char* buffer = new char[nFullBufferLen];
				
				BITMAPFILEHEADER bfh;
				MEMSET( &bfh, 0, sizeof( bfh ) );
				bfh.bfType = 'MB';
				bfh.bfSize = nFullBufferLen;
				bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );
				MEMCOPY(buffer,&bfh,sizeof( bfh ) );

				try {
					hr = pDet->GetBitmapBits(dStartTime+ (nFramesGrabbed*TIMEBETWEENFRAMES), NULL, buffer + sizeof( bfh ), width, height);
				}
				catch (...) {
					hr = E_FAIL;
				}
				if (SUCCEEDED(hr))
				{
					// decode
					CxImage* imgResult = new CxImage();
					imgResult->Decode((BYTE*)buffer, nFullBufferLen, CXIMAGE_FORMAT_BMP);
					delete[] buffer;
					if (!imgResult->IsValid()){
						delete imgResult;
						break;
					}

					// resize if needed
					if (nMaxWidth > 0 && nMaxWidth < width){
						float scale = (float)nMaxWidth / imgResult->GetWidth();
						int nMaxHeigth = imgResult->GetHeight() * scale;
						imgResult->Resample(nMaxWidth, nMaxHeigth, 0);
					}
					
					// decrease bpp if needed
					if (bReduceColor){
						CQuantizer q(256,8);
						q.ProcessImage(imgResult->GetDIB());
						RGBQUAD* ppal=(RGBQUAD*)malloc(256*sizeof(RGBQUAD));
						q.SetColorTable(ppal);
						imgResult->DecreaseBpp(8, true, ppal);
						free(ppal);
					}
					
					CString TestName;
					TestName.Format("G:\\testframe%i.png",nFramesGrabbed);
					//imgResult->Save(TestName,CXIMAGE_FORMAT_PNG);
					// done
					imgResults[nFramesGrabbed] = imgResult;
				}
				else{
					delete[] buffer;
					break;
				}
			}
		}
		return nFramesGrabbed;
	}
	catch(...){
		return 0;
	}
}

void CFrameGrabThread::SetValues(CKnownFile* in_pOwner, CString in_strFileName,uint8 in_nFramesToGrab, double in_dStartTime, bool in_bReduceColor, uint16 in_nMaxWidth, void* in_pSender){
	strFileName =in_strFileName;
	nFramesToGrab = in_nFramesToGrab;
	dStartTime = in_dStartTime;
	bReduceColor = in_bReduceColor;
	nMaxWidth = in_nMaxWidth;
	pOwner = in_pOwner;
	pSender = in_pSender;
}

BEGIN_MESSAGE_MAP(CFrameGrabThread, CWinThread)
END_MESSAGE_MAP()
