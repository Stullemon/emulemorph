#pragma once

class CWebServices
{
public:
	CWebServices();

	CString GetDefaultServicesFile() const;
	int ReadAllServices();
	void RemoveAllServices();

	int GetAllMenuEntries(CMenu& rMenu);
	bool RunURL(const CAbstractFile* file, UINT uMenuID);

protected:
	struct SEd2kLinkService
	{
		CString strMenuLabel;
		CString strUrl;
	};
	CArray<SEd2kLinkService, SEd2kLinkService> m_aServices;
	time_t m_tDefServicesFileLastModified;
};

extern CWebServices theWebServices;
