For create the 16 and 32 bits DLL of Zlib 1.14

For the 16 bits :
unzip zlib114.zip and copy file from build16 in the same directory
open zlib16.mak with Microsoft Visual C++ 1.52


For the 32 bits :
unzip zlib114.zip and copy file from build32 in the same directory

If you use Visual C++ 6, copy file from build32\msvc6
If you use Visual C++ 7 (Visual Studio .NET), copy file from build32\msvc7
If you are using Dec Alpha, copy CRTDLL.DEC under name CRTDLL.LIB, and 
select target ReleaseAxp
If you are using x86, use target Release
open zlibvc.dsw with Microsoft Visual C++ 5.0 or 6.0


Note : Don't use the file zconf.h from zlib114.zip, but use the version
   from buildzlib114dll.zip.
   I've added in buildzlib114dll.zip the version 0.18 of zip/unzip library
   (see http://www.winimage.com/zLibDll/unzip.html for info and update)


Note : You don't need recompile yourself. There is compiled .LIB in
  http://www.winimage.com/zLibDll
