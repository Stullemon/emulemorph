//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#pragma once

#define ACTION_SETUPL		1
#define ACTION_SETDOWNL		2
#define ACTION_SOURCESL		3
#define ACTION_CON5SEC		4
#define ACTION_CONS			5
#define ACTION_CATSTOP		6
#define ACTION_CATRESUME	7
//EastShare START - Added by Pretender, add USS settings in scheduler tab
#define ACTION_USSMAXPING	8
#define ACTION_USSGOUP		9
#define ACTION_USSGODOWN	10
#define ACTION_USSMINUP		11
//EastShare END - Added by Pretender, add USS settings in scheduler tab


#define DAY_DAYLY		0
#define DAY_MO			1
#define DAY_DI			2
#define DAY_MI			3
#define DAY_DO			4
#define DAY_FR			5
#define DAY_SA			6
#define DAY_SO			7
#define DAY_MO_FR		8
#define DAY_MO_SA		9 
#define DAY_SA_SO		10

struct Schedule_Struct{
   CString			title;
   bool				enabled;
   uint8			day;
   uint32			time;
   uint32			time2;
   CString			values[16];
   int				actions[16];
   void ResetActions()	{for (uint8 index=0;index<16;index++) {actions[index]=0;values[index]="";}}
   ~Schedule_Struct() {  }
};

class CScheduler
{
public:
	CScheduler();
	~CScheduler();
	uint8	AddSchedule(Schedule_Struct* schedule);

	void	UpdateSchedule(uint8 index,Schedule_Struct* schedule) {if (index<schedulelist.GetCount())schedulelist.SetAt(index,schedule);}

	Schedule_Struct* GetSchedule(uint8 index) {if (index<schedulelist.GetCount()) return schedulelist.GetAt(index); else return NULL; }
	void	RemoveSchedule(uint8 index);
	void	RemoveAll();
	int		LoadFromFile();
	void	SaveToFile();
	int		Check(bool forcecheck=false);
	uint8	GetCount()		{ return schedulelist.GetCount();}
	void	SaveOriginals();
	void	RestoreOriginals();
	void	ActivateSchedule(uint8 index,bool makedefault=false);
	
	uint16	original_upload;
	uint16	original_download;
	uint16	original_connections;
	uint16	original_cons5s;
	uint16	original_sources;
	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	uint16	original_ussmaxping;
	uint16	original_ussgoup;
	uint16	original_ussgodown;
	uint16	original_ussminup;
	//EastShare END - Added by Pretender, add USS settings in scheduler tab

private:
	CArray<Schedule_Struct*,Schedule_Struct*> schedulelist;
	uint8	m_iLastCheckedMinute;
};

