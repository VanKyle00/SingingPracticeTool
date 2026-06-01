; Inno Setup script for SingingPracticeTool (Windows).
;
; Build flow:
;   1. cmake --build build --config Release --target App
;   2. py sidecar\build_sidecar.py
;   3. py packaging\windows\stage.py
;   4. iscc packaging\windows\installer.iss
;
; Output: packaging\windows\out\SingingPracticeTool-Setup-<version>.exe

#define MyAppName       "SingingPracticeTool"
#define MyAppVersion    "0.1.1"
#define MyAppPublisher  "SingingPracticeTool"
#define MyAppURL        "https://github.com/VanKyle00/SingingPracticeTool"
#define MyAppExeName    "SingingPracticeTool.exe"
#define StageDir        "stage"

[Setup]
AppId={{B5E37D14-9B5A-4E1F-9A1A-2C9B7A1D6D31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
LicenseFile={#SourcePath}\..\..\LICENSE
OutputDir={#SourcePath}\out
OutputBaseFilename=SingingPracticeTool-Setup-{#MyAppVersion}
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile={#SourcePath}\..\..\resources\Icon.ico
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Host exe + everything staged by stage.py. Recurse pulls the practiceml/ bundle dir.
Source: "{#SourcePath}\{#StageDir}\*";  DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: postinstall nowait skipifsilent

[UninstallDelete]
; The host writes device_settings.xml to %APPDATA%\SingingPracticeTool on every device change.
; Leave the user's recordings/stems/MIDI in ~\Music\SingingPracticeTool — those are their work, not ours.
Type: filesandordirs; Name: "{userappdata}\SingingPracticeTool"

[Code]
function IsVCRuntimeInstalled(): Boolean;
var
  Version: String;
begin
  // x64 VS 2015–2022 redistributable.
  Result := RegQueryStringValue (HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', Version);
end;

procedure CurStepChanged (CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if (CurStep = ssPostInstall) and (not IsVCRuntimeInstalled()) then
  begin
    MsgBox ('Microsoft Visual C++ 2015-2022 Redistributable was not detected.' + #13#10 +
            'You can install it from https://aka.ms/vs/17/release/vc_redist.x64.exe',
            mbInformation, MB_OK);
    ResultCode := 0;
  end;
end;
