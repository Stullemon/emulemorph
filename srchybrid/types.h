#ifndef TYPES_H
#define TYPES_H

typedef unsigned char		int8;
typedef unsigned short		int16;
typedef unsigned int		int32;
typedef unsigned char		uint8;
typedef  signed  char		sint8;
typedef unsigned short		uint16;
typedef  signed  short		sint16;
typedef unsigned int		uint32;
typedef  signed  int		sint32;
typedef unsigned __int64	int64;
typedef unsigned __int64	uint64;
typedef signed __int64		sint64;
typedef unsigned long		ulong;
typedef unsigned short		ushort;
typedef unsigned char		uchar;
template <typename type> void safe_delete(type &del) { if(del) { delete del; del=0; } }
#endif
