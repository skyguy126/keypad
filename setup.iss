#define MyAppName "KeypadHost"
#define MyAppVersion "1.0"
#define MyAppURL "https://github.com/skyguy126/keypad"

[Setup]
AppId={{22A04723-351C-490F-A1CA-DFF1C0E02E18}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=E:\Projects\Keypad\export
OutputBaseFilename=setup_keypad
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "E:\Projects\Keypad\export\keypad_host.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Projects\Keypad\export\nircmd.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Projects\Keypad\export\toast.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Projects\Keypad\export\wmicom3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Projects\Keypad\export\signed_driver\*"; DestDir: "{app}\signed_driver\"; Flags: ignoreversion recursesubdirs

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "keypad_host"; ValueData: """{app}\keypad_host.exe"""; Flags: uninsdeletevalue

[Run]
Filename: "{app}\signed_driver\dpinst-x86.exe"; StatusMsg: "Installing Sparkfun ProMicro (x64) driver"; Check: Not IsWin64(); Flags: skipifsilent
Filename: "{app}\signed_driver\dpinst-amd64.exe"; StatusMsg: "Installing Sparkfun ProMicro (x86) driver"; Check: IsWin64(); Flags: skipifsilent
Filename: {app}\{cm:AppName}.exe; Description: {cm:LaunchProgram,{cm:AppName}}; Flags: nowait postinstall skipifsilent

[CustomMessages]
AppName=keypad_host
LaunchProgram=Start after installation

[UninstallRun]
Filename: "{cmd}"; Parameters: "/C ""taskkill /im keypad_host.exe /f /t"

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
