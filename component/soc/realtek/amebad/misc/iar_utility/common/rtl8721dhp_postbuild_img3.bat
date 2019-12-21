cd /D %2
set tooldir=%2\..\..\..\component\soc\realtek\amebad\misc\iar_utility\common\tools
set libdir=%2\..\..\..\component\soc\realtek\amebad\misc\bsp

echo input1=%1 >tmp.txt
echo input2=%2 >>tmp.txt

del Debug\Exe\km4_image\km4_secure.map Debug\Exe\km4_image\km4_secure.asm *.bin
cmd /c "%tooldir%\nm Debug/Exe/km4_image/km4_secure.axf | %tooldir%\sort > Debug/Exe/km4_image/km4_secure.map"
cmd /c "%tooldir%\objdump -d Debug/Exe/km4_image/km4_secure.axf > Debug/Exe/km4_image/km4_secure.asm"

for /f "delims=" %%i in ('cmd /c "%tooldir%\grep IMAGE3 Debug/Exe/km4_image/km4_secure.map | %tooldir%\grep Base | %tooldir%\gawk '{print $1}'"') do set ram3_s_start=0x%%i
for /f "delims=" %%i in ('cmd /c "%tooldir%\grep IMAGE3 Debug/Exe/km4_image/km4_secure.map | %tooldir%\grep Limit | %tooldir%\gawk '{print $1}'"') do set ram3_s_end=0x%%i

for /f "delims=" %%i in ('cmd /c "%tooldir%\grep CMSE Debug/Exe/km4_image/km4_secure.map | %tooldir%\grep Base | %tooldir%\gawk '{print $1}'"') do set ram3_nsc_start=0x%%i
for /f "delims=" %%i in ('cmd /c "%tooldir%\grep CMSE Debug/Exe/km4_image/km4_secure.map | %tooldir%\grep Limit | %tooldir%\gawk '{print $1}'"') do set ram3_nsc_end=0x%%i

echo ram3_s_start: %ram3_s_start% > tmp.txt
echo ram3_s_end: %ram3_s_end% >> tmp.txt
echo ram3_nsc_start: %ram3_nsc_start% >> tmp.txt
echo ram3_nsc_end: %ram3_nsc_end% >> tmp.txt

@echo off&setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /n /c:"PLACEMENT" Debug\List\km4_secure\km4_secure.map') do (
   set skipline=%%i
)
@echo off&setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /n /c:"Kind" Debug\List\km4_secure\km4_secure.map') do (
    set endline=%%i
)
set /a line=endline-skipline

@echo off&setlocal enabledelayedexpansion
set n=0
(for /f "skip=%skipline% delims=" %%a in (Debug\List\km4_secure\km4_secure.map) do (
set /a n+=1
if !n! leq %line% echo %%a
))>km4_secure1.txt

(for /f "delims=" %%a in (km4_secure1.txt) do (
set /p="%%a"<nul | find /V "<Block>"
))>km4_secure2.txt

@echo off&setlocal enabledelayedexpansion
set strstart={
set strend=}
set /a m=1
(for /f "delims=" %%a in (km4_secure2.txt) do (
set /p="%%a"<nul
echo %%a | find "%strstart%" >nul && set /a m-=1
echo %%a | find "%strend%" >nul && set /a m+=1
if !m!==1 (echo.)
))>km4_secure3.txt
findstr /rg "place" km4_secure3.txt > tmp.txt

del km4_secure1.txt
del km4_secure2.txt
del km4_secure3.txt

::findstr /rg "place" Debug\List\km4_secure\km4_secure.map > tmp.txt
setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /rg "IMAGE3" tmp.txt') do (
    set "var=%%i"
    set "sectname_ram3=!var:~1,2!"
)
for /f "delims=:" %%i in ('findstr /rg ".nsc.text" tmp.txt') do (
    set "var=%%i"
    set "sectname_ram3_nsc=!var:~1,2!"
)

setlocal disabledelayedexpansion
del tmp.txt
::echo sectname_ram3: %sectname_ram3% sectname_ram3_nsc: %sectname_ram3_nsc% >tmp1.txt

%tooldir%\objcopy -j "%sectname_ram3% rw" -Obinary Debug/Exe/km4_image/km4_secure.axf Debug/Exe/km4_image/ram3_s.r.bin
%tooldir%\objcopy -j "%sectname_ram3_nsc% rw" -Obinary Debug/Exe/km4_image/km4_secure.axf Debug/Exe/km4_image/ram3_nsc.bin

:: remove bss sections
%tooldir%\pick %ram3_s_start% %ram3_s_end% Debug\Exe\km4_image\ram3_s.r.bin Debug\Exe\km4_image\ram3_s.bin raw

:: add header
%tooldir%\pick %ram3_s_start% %ram3_s_end% Debug\Exe\km4_image\ram3_s.bin Debug\Exe\km4_image\ram3_s.p.bin
%tooldir%\pick %ram3_nsc_start% %ram3_nsc_end% Debug\Exe\km4_image\ram3_nsc.bin Debug\Exe\km4_image\ram3_nsc.p.bin

copy /b Debug\Exe\km4_image\ram3_s.p.bin+Debug\Exe\km4_image\ram3_nsc.p.bin Debug\Exe\km4_image\km4_image3_all.bin
::copy /b Debug\Exe\km4_image\km0_km4_image2.bin+Debug\Exe\km4_image\km4_image3_all.bin Debug\Exe\km4_image\km4_image_all.bin

del Debug\Exe\km4_image\ram3_s.r.bin
del Debug\Exe\km4_image\ram3_s.bin
del Debug\Exe\km4_image\ram3_s.p.bin
del Debug\Exe\km4_image\ram3_nsc.bin
del Debug\Exe\km4_image\ram3_nsc.p.bin

copy Debug\Exe\km4_image\cmse_implib.a %libdir%\lib\common\IAR\cmse_implib.a

exit
