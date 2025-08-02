; Inno Setup Script for Friede
; Save this file as "friede.iss" in your project's root folder (one level above "build").

[Setup]
; Unique ID for your application.
; Generate a new one using the "Tools -> Generate GUID" menu in the Inno Setup editor.
AppId={{11A7E9A3-9E3E-4A8E-A329-3B2E0533A899}

; The name of your application.
AppName=Friede

; The version of this specific build. You will update this for each new release.
AppVersion=1.1.2

; Default directory name. For portable apps, this is less critical but still good practice.
DefaultDirName={autopf}\Friede

; We don't want a start menu folder for a portable app.
DefaultGroupName=Friede
DisableProgramGroupPage=yes

; The name of the final installer executable.
OutputBaseFilename=friede-setup-v1.1.2

; Compression settings for a smaller installer size.
Compression=lzma
SolidCompression=yes

; --- Key Settings for Portable & Silent Updates ---
; Allows the installer to run without showing any UI.
; WinSparkle will use this by passing the /SILENT or /VERYSILENT flag.
PrivilegesRequired=lowest
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; This section tells the installer which files to bundle from your 'build' directory.
; The source path is relative to where this script is saved.
; "{app}" is a constant representing the installation directory.

; The main executable.
Source: "build\friede.exe"; DestDir: "{app}"; Flags: ignoreversion

; All the necessary DLLs from your screenshot.
Source: "build\d3dcompiler_47.dll"; DestDir: "{app}"
Source: "build\Qt6Core.dll"; DestDir: "{app}"
Source: "build\Qt6Gui.dll"; DestDir: "{app}"
Source: "build\Qt6Network.dll"; DestDir: "{app}"
Source: "build\Qt6Pdf.dll"; DestDir: "{app}"
Source: "build\Qt6Svg.dll"; DestDir: "{app}"
Source: "build\Qt6Widgets.dll"; DestDir: "{app}"

; Your asset folders.
Source: "build\banners\*"; DestDir: "{app}\banners"; Flags: recursesubdirs createallsubdirs
Source: "build\icons\*"; DestDir: "{app}\icons"; Flags: recursesubdirs createallsubdirs

; --- CRITICAL QT PLUGIN FOLDERS ---
; These are required for the application to function correctly on other machines.
Source: "build\platforms\*"; DestDir: "{app}\platforms"; Flags: recursesubdirs createallsubdirs
Source: "build\styles\*"; DestDir: "{app}\styles"; Flags: recursesubdirs createallsubdirs
Source: "build\imageformats\*"; DestDir: "{app}\imageformats"; Flags: recursesubdirs createallsubdirs
Source: "build\iconengines\*"; DestDir: "{app}\iconengines"; Flags: recursesubdirs createallsubdirs
Source: "build\tls\*"; DestDir: "{app}\tls"; Flags: recursesubdirs createallsubdirs
Source: "build\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: recursesubdirs createallsubdirs


[Run]
; This section tells the installer what to do after the installation is complete.
; It will run your new friede.exe.
Filename: "{app}\friede.exe"; Description: "{cm:LaunchProgram,Friede}"; Flags: nowait postinstall
