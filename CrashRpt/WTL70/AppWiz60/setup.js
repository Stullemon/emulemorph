// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

// Setup program for the WTL App Wizard for VC++ 6.0

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

	var strSourceFolder = strValue;
	if(bDebug)
		WScript.Echo("Source: " + strSourceFolder);

	var strVC6Key = "HKLM\\Software\\Microsoft\\VisualStudio\\6.0\\Setup\\VsCommonDir";
	try
	{
		strValue = WSShell.RegRead(strVC6Key);
	}
	catch(e)
	{
		WScript.Echo("ERROR: Cannot find where Visual Studio 6.0 is installed.");
		return;
	}

	var strDestFolder = strValue + "\\MSDev98\\Template";
	if(bDebug)
		WScript.Echo("Destination: " + strDestFolder);
	if(!FileSys.FolderExists(strDestFolder))
	{
		WScript.Echo("ERROR: Cannot find destination folder (should be: " + strDestFolder + ")");
		return;
	}

	// Copy App Wizard file
	try
	{
		var strSrc = strSourceFolder + "\\AtlApp60.awx";
		var strDest = strDestFolder + "\\";
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

	WScript.Echo("App Wizard successfully installed!");
}
