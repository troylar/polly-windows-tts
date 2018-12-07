; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Amazon Polly TTS for Windows"
#define MyAppVersion ".1"
#define MyAppPublisher "Amazon Web Services"
#define MyAppURL "https://aws.amazon.com/polly"
#define ReleaseOrDebug "Debug"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{A6481E88-FD76-4BE2-BDF9-BB38F8A37B95}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}                  
OutputBaseFilename=setup
OutputDir=installer
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
LicenseFile=license.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: ".\installer\InstallVoices.exe"; DestDir: "{app}"; Flags: ignoreversion 64bit; Check: IsWin64
Source: ".\installer\PollyWindowsTTS.dll"; DestDir: "{app}"; Flags: ignoreversion regserver 64bit; Check: IsWin64

[Run]
Filename: "{app}\InstallVoices.exe"; Flags: runascurrentuser; Parameters: "install"; StatusMsg: "Installing Voices..."

[UninstallRun]
Filename: "{app}\InstallVoices.exe"; Parameters: "uninstall"
