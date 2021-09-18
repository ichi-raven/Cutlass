[Setup]
AppName=Cutlass v0.9
AppVersion=0.9
AppVerName=Cutlass(0.9)
OutputBaseFilename=Cutlass(0.9)Installer
DefaultDirName={userdocs}\Cutlass(0.9)
AppendDefaultDirName=yes
;SetupIconFile=Icon.ico
VersionInfoVersion=0.9.2
UninstallDisplayIcon={uninstallexe}
ChangesEnvironment=yes
AppPublisher=ichi-raven
AppPublisherURL=https://github.com/ichi-raven/Cutlass
;MinVersion=6.1.7601

[Files]
Source: "D:\Repositories\CutlassWinDist\*"; DestDir: "{app}"; Flags: recursesubdirs;
Source: "D:\Repositories\CutlassWinDist\CutlassApp.zip"; DestDir: "{userdocs}\Visual Studio 2019\Templates\ProjectTemplates";

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType:expandsz; ValueName:"CUTLASS"; ValueData:"{app}" ; Flags: uninsdeletevalue;

[Languages]
Name: english; MessagesFile: compiler:Default.isl
;Name: japanese; MessagesFile: compiler:Languages\Japanese.isl

;[Code]
;procedure CurStepChanged(CurStep: TSetupStep);
;var
;  VSInstallPath: String;
;  ResultCode: Integer;
;begin
;  if CurStep = ssPostInstall then
;  begin
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Community)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Professional)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Enterprise)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;     if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Community)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Professional)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Enterprise)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;  end
;end;
;procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
;var
;  VSInstallPath: String;
;  ResultCode: Integer;
;begin
;  if CurUninstallStep = usPostUninstall then
;  begin
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Community)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Professional)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (C: Enterprise)');
;      Exec('C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;     if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Community)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Professional)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;
;    if FileExists('D:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe') then
;    begin
;      Log('Visual Studio 2019 found (D: Enterprise)');
;      Exec('D:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe', '/InstallVSTemplates', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
;    end;   
;  end
;end;