#pragma once
#include "PartFile.h"
//#include "SR13-ImportParts.rc.h"

#define	MP_SR13_ImportParts			13000	/* Rowaa[SR13]: Import parts to file */
#define	MP_SR13_InitiateRehash		13001	/* Rowaa[SR13]: Initiates file rehash */

#define GetFileNameorInfo(X)	GetFileName()

void SR13_InitiateRehash(CPartFile* partfile);

class CImportPartsFileThread : public CWinThread{
	DECLARE_DYNCREATE(CImportPartsFileThread)
protected:
	CImportPartsFileThread();
public:
	virtual BOOL InitInstance();
	virtual int		Run();
	void	SetValues(LPCTSTR szfile, CPartFile* partfile, bool* inprogress);
private:
	CPartFile*		m_partfile;
	CString			m_strFilePath;
	bool*			bInProgress;
};
