#pragma once
#include "PartFile.h"

#define	MP_SR13_ImportParts			13000	/* Rowaa[SR13]: Import parts to file */
#define	MP_SR13_InitiateRehash		13001	/* Rowaa[SR13]: Initiates file rehash */

void SR13_InitiateRehash(CPartFile* partfile);

class CImportPartsFileThread : public CWinThread
{
	DECLARE_DYNCREATE(CImportPartsFileThread)
protected:
	CImportPartsFileThread();
public:
	virtual BOOL InitInstance();
	virtual int		Run();
	void	SetValues(LPCTSTR szfile, CPartFile* partfile = NULL);

private:
	CPartFile*		 m_partfile;
	CString			 m_strFilePath;
};