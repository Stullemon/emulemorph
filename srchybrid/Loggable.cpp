#include "StdAfx.h"
#include "emule.h"
#include "loggable.h"

void CLoggable::AddLogLine(bool addtostatusbar, UINT nID, ...) {
		
	va_list argptr;
	va_start(argptr, nID);	
	AddLogText(false, addtostatusbar, GetResString(nID), argptr);	
	va_end(argptr);	
}

void CLoggable::AddLogLine(bool addtostatusbar, LPCTSTR line, ...) {
	
	ASSERT(line != NULL);	

	va_list argptr;
	va_start(argptr, line);	
	AddLogText(false, addtostatusbar, line, argptr);	
	va_end(argptr);
}

void CLoggable::AddDebugLogLine(bool addtostatusbar, UINT nID, ...) {

	va_list argptr;
	va_start(argptr, nID);	
	AddLogText(true, addtostatusbar, GetResString(nID), argptr);			
	va_end(argptr);	
}

void CLoggable::AddDebugLogLine(bool addtostatusbar, LPCTSTR line, ...) {
	
	ASSERT(line != NULL);

	va_list argptr;
	va_start(argptr, line);
	AddLogText(true, addtostatusbar, line, argptr);	
	va_end(argptr);	
}

void CLoggable::AddLogText(bool debug, bool addtostatusbar,LPCTSTR line, va_list argptr) {
	
	ASSERT(line != NULL);


	if (debug && !theApp.glob_prefs->GetVerbose())
		return;	

	const size_t bufferSize = 1000;
	TCHAR bufferline[bufferSize];	
	if (_vsntprintf(bufferline, bufferSize, line, argptr) == -1)
		bufferline[bufferSize - 1] = _T('\0');
	
	if (theApp.emuledlg)
		theApp.emuledlg->AddLogText(addtostatusbar, bufferline, debug);	//Cax2 - debug log and normal log handled by the same subroutine now
#ifdef _DEBUG
	else{
		TRACE("App Log: %s\n", bufferline);
	}
#endif
}

// DbT:Logging
void CLoggable::PacketToDebugLogLine(LPCTSTR info, char * packet, uint32 size, uint8 opcode) const {
	CString buffer; 
	buffer.Format(_T("%s: %02x, size=%u"), info, opcode, size);
	buffer += _T(", data=[");
	uint32 maxsize = 100;
	for(uint32 i = 0; i < size && i < maxsize; i++){		
		buffer.AppendFormat(_T("%02x"), (uint8)packet[i]);
		buffer += _T(" ");
	}
	buffer += ((size < maxsize) ? _T("]") : _T("..]"));
	AddDebugLogLine(false, buffer); 
}

void CLoggable::TagToDebugLogLine(LPCTSTR info, LPCTSTR tag, uint32 size, uint8 opcode) const {
	CString buffer; 
	buffer.Format(_T("%s: %02x, size=%u"), info, opcode, size);
	buffer += _T(", data=[");
	uint32 maxsize = 100;
	for(uint32 i = 0; i < size && i < maxsize; i++){		
		buffer.AppendFormat(_T("%02x"), (uint8)tag[i]);
		buffer += _T(" ");
	}
	buffer += ((size < maxsize) ? _T("]") : _T("..]"));
	AddDebugLogLine(false, buffer); 
}
// DbT:End
