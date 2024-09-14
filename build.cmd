@echo off

rem must has trail slash
set MS="C:\Program Files\Bentley\MicroStation CONNECT Edition\MicroStation\"
set PS="C:\Program Files\Bentley\ProStructures CONNECT Edition\ProStructures\"
set MS_SDK="C:\Program Files\Bentley\MicroStationCONNECTSDK\"
set PS_SDK=C:\Program Files\Bentley\ProStructuresSDK\

set PS_INCLUDE="%PS_SDK%include\Rebar"
set PS_LIBRARY="%PS_SDK%library"
echo %PS_INCLUDE%
echo %PS_LIBRARY%

set PS_SDK="%PS_SDK%"

set SDK_COMMON_ENV_BAT="C:\Program Files\Bentley\MicroStationCONNECTSDK\bin\SDKCommonEnv.bat"

set _SDKTRACE=0

rem init microstation
@REM call %SDK_COMMON_ENV_BAT% %MS% %MS_SDK%

rem init pro structures
set SDKPROD=%PS%
set PP_SDK=%MS%
call %SDK_COMMON_ENV_BAT% %PS% %MS_SDK%

set BMAKE_OPT=-I%SDKMKI%
set BMAKE_OPT=%BMAKE_OPT% -I%PS_SDK%mki\

set MS=%SDKPROD%
set MSMDE=%SDK%
set MSMDE_OUTPUT=%SDKTEMP%
set PATH=%PATH%;%SDKBIN%;%SDKREMAP%;%SDKPROD%
set DEFAULT_TARGET_PROCESSOR_ARCHITECTURE=x64
set DLM_NO_SIGN=1

cd /d %~dp0

bmake -ddebug
@REM bmake -a -dIDE

@REM pause