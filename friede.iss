[Setup]
AppId={{11A7E9A3-9E3E-4A8E-A329-3B2E0533A899}
AppName=Friede
AppVersion=1.2.0
OutputBaseFilename=friede-setup-v1.2.0
DefaultDirName={autopf}\Friede
DefaultGroupName=Friede
DisableProgramGroupPage=yes
Compression=lzma
SolidCompression=yes
PrivilegesRequired=lowest
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "build\*.dll"; DestDir: "{app}"
Source: "build\tls\*"; DestDir: "{app}\tls"; Flags: recursesubdirs createallsubdirs
Source: "build\icons\*"; DestDir: "{app}\icons"; Flags: recursesubdirs createallsubdirs
Source: "build\styles\*"; DestDir: "{app}\styles"; Flags: recursesubdirs createallsubdirs
Source: "build\banners\*"; DestDir: "{app}\banners"; Flags: recursesubdirs createallsubdirs
Source: "build\friede.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\platforms\*"; DestDir: "{app}\platforms"; Flags: recursesubdirs createallsubdirs
Source: "build\iconengines\*"; DestDir: "{app}\iconengines"; Flags: recursesubdirs createallsubdirs
Source: "build\imageformats\*"; DestDir: "{app}\imageformats"; Flags: recursesubdirs createallsubdirs
Source: "build\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: recursesubdirs createallsubdirs

[Run]
Filename: "{app}\friede.exe"; Description: "{cm:LaunchProgram,Friede}"; Flags: nowait postinstall