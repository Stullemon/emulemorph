#pragma once

#include "types.h"

class CLoggable
{
public:
	static void AddLogLine(bool addtostatusbar,UINT nID,...);
	static void AddLogLine(bool addtostatusbar,LPCTSTR line,...);
	static void AddDebugLogLine(bool addtostatusbar, UINT nID,...);
	static void AddDebugLogLine(bool addtostatusbar, LPCTSTR line,...);
// DbT:Logging
	void PacketToDebugLogLine(LPCTSTR info, char * packet, uint32 size, uint8 opcode) const;
	void TagToDebugLogLine(LPCTSTR info, LPCTSTR tag, uint32 size, uint8 opcode) const;
// DbT:End
private:	
	static void AddLogText(bool debug, bool addtostatusbar, LPCTSTR line, va_list argptr);	
};
