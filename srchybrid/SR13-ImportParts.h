#pragma once
#include "PartFile.h"

#define	MP_SR13_ImportParts			13000	/* Rowaa[SR13]: Import parts to file */
#define	MP_SR13_InitiateRehash		13001	/* Rowaa[SR13]: Initiates file rehash */

void SR13_InitiateRehash(CPartFile* partfile);
