REM Compiling Daxanadu
MSBUILD "build\Dax.sln" /t:Daxanadu /p:Configuration="Release" /p:Platform="Win32"
COPY build\bin\Release\Daxanadu.exe Release\Daxanadu.exe

REM Copy over APcpp DLL
COPY build\bin\Release\APCpp.dll Release\APCpp.dll

REM Copy assets
XCOPY /s /y game\assets Release\assets

REM REM Archiving apworld
DEL /F /Q ..\Archipelago\worlds\faxanadu\__pycache__
winrar a -afzip -ep1 -r Release\faxanadu.apworld ..\Archipelago\worlds\faxanadu

REM Generating default yaml
python3 ..\Archipelago\Launcher.py "Generate Template Settings"
COPY "..\Archipelago\Players\Templates\Faxanadu.yaml" "Release\Faxanadu.yaml"

REM Archiving release
winrar a -afzip -ep1 -r Release\Daxanadu_x_x_x_beta.zip @release.lst
