// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

// Setup program for the WTL App Wizard for VC++ 7.0

main();

function main()
{
	var bDebug = false;
	var Args = WScript.Arguments;
	if(Args.length > 0 && Args(0) == "/debug")
		bDebug = true;

	// Create shell object
	var WSShell = WScript.CreateObject("WScript.Shell");
	// Create file system object
	var FileSys = WScript.CreateObject("Scripting.FileSystemObject");

	var strValue = FileSys.GetAbsolutePathName(".");
	if(strValue == null || strValue == "")
		strValue = ".";

	var strSourceFolder = strValue + "\\" + "Files";
	if(bDebug)
		WScript.Echo("Source: " + strSourceFolder);

	if(!FileSys.FolderExists(strSourceFolder))
	{
		WScript.Echo("ERROR: Cannot find Wizard folder (should be: " + strSourceFolder + ")");
		return;
	}

	var strVC7Key = "HKLM\\Software\\Microsoft\\VisualStudio\\7.0\\Setup\\VC\\ProductDir";
	try
	{
		strValue = WSShell.RegRead(strVC7Key);
	}
	catch(e)
	{
		WScript.Echo("ERROR: Cannot find where Visual Studio 7.0 is installed.");
		return;
	}

	var strDestFolder = strValue + "\\vcprojects";
	if(bDebug)
		WScript.Echo("Destination: " + strDestFolder);
	if(!FileSys.FolderExists(strDestFolder))
	{
		WScript.Echo("ERROR: Cannot find destination folder (should be: " + strDestFolder + ")");
		return;
	}

	var strDest = strDestFolder + "\\";
	var strSrc = "";

	// Copy files
	try
	{
		strSrc = strSourceFolder + "\\WTLApp70.ico";
		FileSys.CopyFile(strSrc, strDest);
		strSrc = strSourceFolder + "\\WTLApp70.vsdir";
		FileSys.CopyFile(strSrc, strDest);
	}
	catch(e)
	{
		var strError = "no info";
		if(e.description.length != 0)
			strError = e.description;
		WScript.Echo("ERROR: Cannot copy file (" + strError + ")");
		return;
	}

	// Read and write WTLApp70.vsz, replace path when found
	try
	{
		strSrc = strSourceFolder + "\\WTLApp70.vsz";
		strDest = strDestFolder + "\\WTLApp70.vsz";

		var ForReading = 1;
		var fileSrc = FileSys.OpenTextFile(strSrc, ForReading);
		if(fileSrc == null)
		{
			WScript.Echo("ERROR: Cannot open source file " + strSrc);
			return;
		}

		var ForWriting = 2;
		var fileDest = FileSys.OpenTextFile(strDest, ForWriting, true);
		if(fileDest == null)
		{
			WScript.Echo("ERROR: Cannot open destination file" + strDest);
			return;
		}

		while(!fileSrc.AtEndOfStream)
		{
			var strLine = fileSrc.ReadLine();
			if(strLine.indexOf("ABSOLUTE_PATH") != -1)
				strLine = "Param=\"ABSOLUTE_PATH = " + strSourceFolder + "\"";
			fileDest.WriteLine(strLine);
		}

		fileSrc.Close();
		fileDest.Close();
	}
	catch(e)
	{
		var strError = "no info";
		if(e.description.length != 0)
			strError = e.description;
		WScript.Echo("ERROR: Cannot read and write WTLApp70.vsz (" + strError + ")");
		return;
	}

	WScript.Echo("App Wizard successfully installed!");
}
