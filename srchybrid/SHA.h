/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary 
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright 
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products 
      built using this software without specific written permission. 

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.
 
 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness 
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 30/11/2002

 This is a byte oriented version of SHA1 that operates on arrays of bytes
 stored in memory. It runs at 22 cycles per byte on a Pentium P4 processor
*/

#pragma once
#include "shahashset.h"

class CSHA : public CAICHHashAlgo  
{
// Construction
public:
	CSHA();
	~CSHA();
// Operations
public:
	virtual void	Reset();
	virtual void	Add(LPCVOID pData, DWORD nLength);
	virtual void	Finish(CAICHHash& Hash);
	virtual void	GetHash(CAICHHash& Hash);
protected:
	void			Compile();
private:
	DWORD	m_nCount[2];
	DWORD	m_nHash[5];
	DWORD	m_nBuffer[16];
};

#define SHA1_BLOCK_SIZE		64
#define SHA1_DIGEST_SIZE	20
