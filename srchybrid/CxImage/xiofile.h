#if !defined(__xiofile_h)
#define __xiofile_h

class DLL_EXP CxIOFile : public CxFile
	{
public:
	explicit CxIOFile(FILE* fp = NULL)
	{
		m_fp = fp;
		m_bCloseFile = (bool)(fp==0);
	}

	~CxIOFile()
	{
		CxIOFile::Close();
	}
//////////////////////////////////////////////////////////
	bool Open(const TCHAR * filename, const TCHAR * mode)
	{
		if (m_fp) return false;	// Can't re-open without closing first

		m_fp = _tfopen(filename, mode);
		if (!m_fp) return false;

		m_bCloseFile = true;

		return true;
	}
//////////////////////////////////////////////////////////
	virtual bool Close()
	{
		int32_t iErr = 0;
		if ( (m_fp) && (m_bCloseFile) ){
			iErr = fclose(m_fp);
			m_fp = NULL;
		}
		return (iErr==0);
	}
//////////////////////////////////////////////////////////
	virtual size_t	Read(void *buffer, size_t size, size_t count)
	{
		return m_fp != NULL ? fread(buffer, size, count, m_fp) : 0;
	}
//////////////////////////////////////////////////////////
	virtual size_t	Write(const void *buffer, size_t size, size_t count)
	{
		return m_fp != NULL ? fwrite(buffer, size, count, m_fp) : 0;
	}
//////////////////////////////////////////////////////////
	virtual bool Seek(int32_t offset, int32_t origin)
	{
		return m_fp != NULL && fseek(m_fp, offset, origin) == 0;
	}
//////////////////////////////////////////////////////////
	virtual int32_t Tell()
	{
		return m_fp != NULL ? ftell(m_fp) : 0;
	}
//////////////////////////////////////////////////////////
	virtual int32_t	Size()
	{
		if (!m_fp)
			return -1;
		int32_t pos = ftell(m_fp);
		fseek(m_fp, 0, SEEK_END);
		int32_t size = ftell(m_fp);
		fseek(m_fp, pos, SEEK_SET);
		return size;
	}
//////////////////////////////////////////////////////////
	virtual bool	Flush()
	{
		return m_fp != NULL && fflush(m_fp) == 0;
	}
//////////////////////////////////////////////////////////
	virtual bool	Eof()
	{
		return m_fp == NULL || feof(m_fp) != 0;
	}
//////////////////////////////////////////////////////////
	virtual int32_t	Error()
	{
		return m_fp != NULL ? ferror(m_fp) : -1;
	}
//////////////////////////////////////////////////////////
	virtual bool PutC(uint8_t c)
	{
		return m_fp != NULL && fputc(c, m_fp) == c;
	}
//////////////////////////////////////////////////////////
	virtual int32_t	GetC()
	{
		return m_fp != NULL ? getc(m_fp) : EOF;
	}
//////////////////////////////////////////////////////////
	virtual char *	GetS(char *string, int32_t n)
	{
		return m_fp != NULL ? fgets(string,n,m_fp) : NULL;
	}
//////////////////////////////////////////////////////////
	virtual int32_t	Scanf(const char *format, void* output)
	{
		return m_fp != NULL ? fscanf(m_fp, format, output) : EOF;
	}
//////////////////////////////////////////////////////////
protected:
	FILE *m_fp;
	bool m_bCloseFile;
	};

#endif
