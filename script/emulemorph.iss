; create by leuk_he on 10-01-2006
; (c) 2006-2007 leuk_he (leukhe at gmail dot com)

[Setup]
AppName=eMule
AppVerName=morphemuleversion
AppPublisher=Morph team
AppPublisherURL=http://emulemorph.sourceforge.net/
AppSupportURL=http://forum.emule-project.net/index.php?showforum=28
AppUpdatesURL=http://sourceforge.net/project/showfiles.php?group_id=72158&package_id=107495
AppCopyright=(c) 2007 Morph team.

UsePreviousAppDir=yes
DirExistsWarning=No
DefaultDirName={pf}\eMule
DefaultGroupName=eMule
AllowNoIcons=yes
; gpl:
LicenseFile=..\staging\license.txt
WizardImageFile=MorphBanner.bmp
WizardImageStretch=no

; this dir:
OutputDir=.
OutputBaseFilename=morphemuleversion-installer
SetupIconFile=..\srchybrid\res\mod\installerico.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
;Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl";LicenseFile: "..\staging\license-GER.txt"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl" ;LicenseFile: "..\staging\license-IT.txt"
Name: "spanish"; MessagesFile: "SpanishStd-2-5.1.0.isl" ;LicenseFile: "..\staging\license-SP.txt"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "french";  MessagesFile: "compiler:Languages\French.isl";LicenseFile: "..\staging\license-FR.txt"
Name: "BrazilianPortuguese" ;MessagesFile: "compiler:Languages\BrazilianPortuguese.isl";LicenseFile: "..\staging\license-PT_BR.txt"
Name: "ChineseSimpl" ;MessagesFile: "ChineseSimp-11-5.1.0.isl"

[Dirs]
Name: "{app}\config"        ; Permissions:users-modify
Name: "{app}\temp"          ; Permissions:users-modify
Name: "{app}\incoming"      ; Permissions:users-modify

[Files]
;todo show correct languge in startup
Source: "..\staging\emule.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\Template.eMuleSkin.ini"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\changelog.ger.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\Changelog.MorphXT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\changelog.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\emule.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-DK.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-FR.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-GER.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-GR.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-HE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-IT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-KO.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-LT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-PT_BR.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-PT_PT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-RU.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-SP.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\license-TR.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\eMule.chm"; DestDir: "{app}"; Flags: ignoreversion  onlyifdoesntexist
Source: "..\staging\eMule.1031.chm"; DestDir: "{app}"; Flags: ignoreversion  onlyifdoesntexist
Source: "..\staging\readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\unrar.dll"; DestDir: "{app}"
Source: "..\staging\unrarlicense.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\mediainfo.dll"; DestDir: "{app}";  Flags: ignoreversion onlyifdoesntexist
Source: "..\staging\mediainfo_ReadMe_DLL.txt"; DestDir: "{app}"; Flags: ignoreversion  onlyifdoesntexist
Source: "..\staging\emule\config\AC_ServerMetURLs.dat"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
Source: "..\staging\emule\config\AC_SearchStrings.dat"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist  ; Permissions:users-modify
Source: "..\staging\emule\config\countryflag.dll"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\countryflag32.dll"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\eMule Light.tmpl"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\Multiuser eMule.tmpl"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\eMule.tmpl"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\startup.wav"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\webcaches.xml"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\webservices.dat"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\XMLNews.dat"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\staging\emule\config\server.met"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
Source: "..\staging\emule\config\addresses.dat"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
Source: "..\staging\emule\config\ip-to-country.csv"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
Source: "..\staging\emule\config\ipfilter.dat"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
Source: "..\staging\emule\config\staticservers.dat"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist ; Permissions:users-modify
; to assing correct rights:
Source: "..\staging\emule\config\preferences.ini"; DestDir: "{app}\config"; Flags: ignoreversion onlyifdoesntexist uninsneveruninstall  ; Permissions:users-modify
Source: "..\staging\lang\*"; DestDir: "{app}\lang"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\staging\webserver\*"; DestDir: "{app}\webserver"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\staging\wapserver\*"; DestDir: "{app}\wapserver"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\eMuleShellExt\Release\eMuleShellExt.dll"; DestDir: "{app}" ; Flags: regserver ignoreversion ;Tasks: shellextention
Source: "..\eMuleShellExt\doc\eMule Shell Extension.htm"; DestDir: "{app}" ; Flags: ignoreversion;Tasks: shellextention
Source: "..\eMuleShellExt\doc\eMule Shell Extension-DE.PNG"; DestDir: "{app}" ; Flags: ignoreversion;Tasks: shellextention



; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; flags: unchecked
Name: "firewall"; Description: "{cm:tasks_firewall}"; MinVersion: 0,5.01sp2
Name: "urlassoc"; Description: "{cm:tasks_assocurl}"; GroupDescription: "Other tasks:"
Name: "shellextention"; Description: "{cm:shellextention}"; GroupDescription: "Other tasks:" ; Flags: unchecked ;MinVersion: 0,5.01


[INI]
Filename: "{app}\emulemorphhome.url"; Section: "InternetShortcut"; Key: "URL"; String: "http://emulemorph.sourceforge.net/"

[Icons]
Name: "{group}\eMule MorphXT"; Filename: "{app}\emule.exe" ; Comment: "eMule MorphXT"
Name: "{group}\{cm:ProgramOnTheWeb,eMule Morph}"; Filename: "{app}\emulemorphhome.url"
Name: "{group}\{cm:UninstallProgram,eMule}"; Filename: "{uninstallexe}"
Name: "{group}\Shell extension doc" ; Filename: "{app}\eMule Shell Extension.htm"; Tasks: shellextention
Name: "{userdesktop}\eMule"; Filename: "{app}\emule.exe"; Tasks: desktopicon

[Registry]
Root: HKCR; Subkey: "ed2k"; ValueType: string; ValueData: "URL:ed2k Protocol"; Flags: uninsdeletekey; Tasks: urlassoc
Root: HKCR; Subkey: "ed2k"; ValueName: "URL Protocol"; ValueType: string; Flags: uninsdeletekey;  Tasks: urlassoc
Root: HKCR; Subkey: "ed2k\Shell"; ValueType: string; Flags: uninsdeletekey;  Tasks: urlassoc
Root: HKCR; Subkey: "ed2k\Shell\open"; ValueType: string; Flags: uninsdeletekey;  Tasks: urlassoc
Root: HKCR; Subkey: "ed2k\Shell\open\Command"; ValueType: string; ValueData: "{app}\emule.exe ""%1"""; Flags: uninsdeletekey;  Tasks: urlassoc

Root: HKCR; Subkey: ".emulecollection"; ValueType: string; ValueName: ""; ValueData: "eMule"; Flags: uninsdeletevalue;Tasks: urlassoc
Root: HKCR; Subkey: "eMule"; ValueType: string; ValueName: ""; ValueData: "eMule"; Flags: uninsdeletekey;  Tasks: urlassoc
Root: HKCR; Subkey: "eMule\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\eMule.exe,1";  Tasks: urlassoc
Root: HKCR; Subkey: "eMule\shell\open\Command"; ValueType: string; ValueData: "{app}\emule.exe ""%1"""; Flags: uninsdeletekey;  Tasks: urlassoc
; shell extension uninstall:
Root: HKCR; Subkey: ".met"; ValueType: string; ValueName: ""; ValueData: "metfile "; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: ".part"; ValueType: string; ValueName: ""; ValueData: "partfile "; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "CLSID\{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}"; ValueName: ""; ValueData: "eMule shell extension";  ValueType: string; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "CLSID\{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}\InprocServer32"; ValueName: ""; ValueData: "{app}\eMuleShellExt.dll";  ValueType: string; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "CLSID\{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}\InprocServer32"; ValueName: "ThreadingModel"; ValueData: "Both";  ValueType: string; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "metfile"; ValueType: string; ValueName: "Details"; ValueData: "prop:DocTitle;Artist;Album;Duration;Bitrate;Write"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "metfile"; ValueType: string; ValueName: "infoTip"; ValueData: "prop:DocTitle;Artist;Album;Duration;Bitrate;Write"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "metfile"; ValueType: string; ValueName: "Tileinfo"; ValueData: "prop:DocTitle;Size"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "metfile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\eMuleShellExt.dll,-201"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile\ShellEx\PropertyHandler"; ValueType: string; ValueName: "" ;ValueData: "{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}}";   Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile"; ValueType: string; ValueName: "Details"; ValueData: "prop:DocTitle;Artist;Album;Duration;Bitrate;Write"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile"; ValueType: string; ValueName: "infoTip"; ValueData: "prop:DocTitle;Artist;Album;Duration;Bitrate;Write"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile"; ValueType: string; ValueName: "Tileinfo"; ValueData: "prop:DocTitle;Size"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\eMuleShellExt.dll,-202"; Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "partfile\ShellEx\PropertyHandler"; ValueType: string; ValueName: "" ;ValueData: "{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}";   Flags: uninsdeletevalue;Tasks: shellextention
Root: HKCR; Subkey: "Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"; ValueType: string; ValueName: "{{5F081689-CE7D-43E7-8B11-DAD99A4A96D6}" ;ValueData: "eMule shell extension";   Flags: uninsdeletevalue;Tasks: shellextention


[Run]
;Not in vista because install runs as admin.
Filename: "{app}\emule.exe"; Description: "{cm:LaunchProgram,eMule}"; Flags: nowait postinstall skipifsilent ;OnlyBelowVersion: 0,6;
Filename: "netsh"; PArameters:"firewall add allowedprogram ""{app}\emule.exe"" eMuleMorphXT enable all" ; flags: runminimized; Tasks: firewall

[uninstallrun]
Filename: "{app}\emule.exe"; Parameters: "uninstall"
Filename: "netsh"; PArameters:"firewall delete allowedprogram ""{app}\emule.exe""" ; flags: runminimized ; Tasks: firewall


[UninstallDelete]
Type: files; Name: "{app}\emulemorphhome.url"

[CustomMessages]
; This section specifies phrazes and words not specified in the ISL files
; Avoid customizing the ISL files since they will change with each version of Inno Setup.
; English
tasks_firewall=Add an exception to the Windows Firewall
dialog_firewall=Setup failed to add eMule to the Windows Firewall.%nPlease add eMule to the exception list manually.
;dutch.tasks_firewall=Voeg een uitzonderings regel toe aan de windows firewall.
;dutch.dialog_firewall=Setup heeft emule niet als uitzondering aan de Windows Firewall kunnen toevoegen .%nWellicht moet u dit nog handmatig doen.
spanish.tasks_firewall=A?adir una excepción al Cortafuegos de Windows
spanish.dialog_firewall=No se pudo a?adir eMule al Cortafuegos de Windows.%nA?ada el eMule a la lista del cortafuegos manualmente.
french.tasks_firewall=Ajouter une exception dans le Pare-feu Windows
french.dialog_firewall=mpossible d'ajouter emule dans le firewall de windows.%nMerci d'ajouter  emule dans la liste du firewall de windows.
german.tasks_firewall=Eine Ausnahme für Windows Firewall erstellen
german.dialog_firewall=Setup konnte keine Ausnamhe für eMule in der Windows Firewall hinzufügen.%nBitte eMule manuell auf die Liste der Ausnahmen setzen.
BrazilianPortuguese.tasks_firewall=Adiciona uma exce??o ao Firewall do Windows
BrazilianPortuguese.dialog_firewall=O Setup falhou ao adicionar o eMule ao Firewall do Windows.%nPor favor adicione o eMule na lista de exce??es manualmente.
tasks_assocurl=Registers eMule to take ed2k-Links and .emulecollection.
;dutch.assocurl=eD2K links doorsturen naar eMule.
spanish.assocurl=Capturar enlaces Ed2k and .emulecollection.
french.assocurl=Associer avec les liens Ed2k et .emulecollection.
german.assocurl=Ed2k-Links nehmen und  .emulecollection.
BrazilianPortuguese.assocurl=BrazilianPortuguese.assocurl=Associar com links ED2K e arquivos .emulecollection
ChineseSimpl.assocurl="?M Ed2k 3sμ22￡￥í??áp"
shellextention=Install shell extention for .met and .part files.
italian.shellextention=Installa shell extention per i file .met e .part.
spanish.shellextention=en el núcleo, los archivos con extensiones .met y .part.
BrazilianPortuguese.shellextention=Instalar a extens鉶 shell para os ficheiros .met e .part
ChineseSimpl.shellextention=让游览器为.met和.part文件显示更多信息。

; Code sections need to be the last section in a script or the compiler will get confused
[Code]
const
  WM_CLOSE = $0010;
  NET_FW_SCOPE_ALL = 0;
  NET_FW_IP_VERSION_ANY = 2;
var
  //Installed: Boolean;
  FirewallFailed: string;

Procedure CurStepChanged(CurStep: TSetupStep);
var
  InstallFolder: string;
  pref: string;
  FirewallObject: Variant;
  FirewallManager: Variant;
  FirewallProfile: Variant;
  //Reset: boolean;
Begin
  if CurStep=ssPostInstall then begin
  // debug:
      pref := ExpandConstant('{app}\config\preferences.ini');

    if CompareText(activelanguage,'dutch')=0 then
       if  not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1043',pref );
       end;
    if CompareText(activelanguage,'german')=0 then
       if not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1031',pref );
       end;
   if CompareText(activelanguage,'italian')=0 then
       if not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1040',pref );
       end;
           if CompareText(activelanguage,'polish')=0 then
       if not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1045',pref );
       end;
    if CompareText(activelanguage,'spanish')=0 then
       if  not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1034',pref );
       end;
    if CompareText(activelanguage,'french')=0 then
       if  not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1036',pref );
       end;
     if CompareText(activelanguage,'BrazilianPortuguese')=0 then
       if  not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','1046',pref );
       end;
     if CompareText(activelanguage,'ChineseSimpl')=0 then
       if  not IniKeyExists('eMule', 'Language', pref) then
       begin
          SetIniString('eMule', 'Language','2052',pref );
       end;
      //
    if  not IniKeyExists('eMule', 'SetSystemACP', pref)  then
        if  not FileExists('{app}\known.met') then
        begin
        // for a new installion system uni code is recommended anyway:
            SetIniString('eMule', 'SetSystemACP','0',pref );
        end;
   end;

//  ;if CurStep=ssDone then Reset := ResetLanguages;
End;




