cd /D %2
set tooldir=%2\..\..\..\component\soc\realtek\amebad\misc\iar_utility\common\tools
set libdir=%2\..\..\..\component\soc\realtek\amebad\misc\bsp
set iartooldir=%3

echo input1=%1 >tmp.txt
echo input2=%2 >>tmp.txt

del Debug\Exe\km0_image\km0_application.map Debug\Exe\km0_image\km0_application.asm Debug\Exe\km0_image\km0_application.dbg.axf
cmd /c "%tooldir%\nm Debug/Exe/km0_image/km0_application.axf | %tooldir%\sort > Debug/Exe/km0_image/km0_application.map"
cmd /c "%tooldir%\objdump -d Debug/Exe/km0_image/km0_application.axf > Debug/Exe/km0_image/km0_application.asm"

for /f "delims=" %%i in ('cmd /c "%tooldir%\grep IMAGE2 Debug/Exe/km0_image/km0_application.map | %tooldir%\grep Base | %tooldir%\gawk '{print $1}'"') do set ram2_start=0x%%i
for /f "delims=" %%i in ('cmd /c "%tooldir%\grep IMAGE2 Debug/Exe/km0_image/km0_application.map | %tooldir%\grep Limit | %tooldir%\gawk '{print $1}'"') do set ram2_end=0x%%i

for /f "delims=" %%i in ('cmd /c "%tooldir%\grep xip_image2 Debug/Exe/km0_image/km0_application.map | %tooldir%\grep Base | %tooldir%\gawk '{print $1}'"') do set xip_image2_start=0x%%i
for /f "delims=" %%i in ('cmd /c "%tooldir%\grep xip_image2 Debug/Exe/km0_image/km0_application.map | %tooldir%\grep Limit | %tooldir%\gawk '{print $1}'"') do set xip_image2_end=0x%%i

echo ram2_start: %ram2_start% > tmp.txt
echo ram2_end: %ram2_end% >> tmp.txt
echo xip_image2_start: %xip_image2_start% >> tmp.txt
echo xip_image2_end: %xip_image2_end% >> tmp.txt

@echo off&setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /n /c:"PLACEMENT" Debug\List\km0_application\km0_application.map') do (
   set skipline=%%i
)
@echo off&setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /n /c:"Kind" Debug\List\km0_application\km0_application.map') do (
    set endline=%%i
)
set /a line=endline-skipline

@echo off&setlocal enabledelayedexpansion
set n=0
(for /f "skip=%skipline% delims=" %%a in (Debug\List\km0_application\km0_application.map) do (
set /a n+=1
if !n! leq %line% echo %%a
))>km0_application1.txt

(for /f "delims=" %%a in (km0_application1.txt) do (
set /p="%%a"<nul | find /V "<Block>"
))>km0_application2.txt

@echo off&setlocal enabledelayedexpansion
set strstart={
set strend=}
set /a m=1
(for /f "delims=" %%a in (km0_application2.txt) do (
set /p="%%a"<nul
echo %%a | find "%strstart%" >nul && set /a m-=1
echo %%a | find "%strend%" >nul && set /a m+=1
if !m!==1 (echo.)
))>km0_application3.txt
findstr /rg "place" km0_application3.txt > tmp.txt

del km0_application1.txt
del km0_application2.txt
del km0_application3.txt

setlocal enabledelayedexpansion
for /f "delims=:" %%i in ('findstr /rg "IMAGE2" tmp.txt') do (
    set "var=%%i"
    set "sectname_ram2=!var:~1,2!"
)
for /f "delims=:" %%i in ('findstr /rg "xip_image2.text" tmp.txt') do (
    set "var=%%i"
    set "sectname_xip=!var:~1,2!"
)

setlocal disabledelayedexpansion
del tmp.txt
::echo sectname_ram2: %sectname_ram2% sectname_xip: %sectname_xip% 

%tooldir%\objcopy -j "%sectname_ram2% rw" -Obinary Debug/Exe/km0_image/km0_application.axf Debug/Exe/km0_image/ram_2.r.bin
%tooldir%\objcopy -j "%sectname_xip% rw" -Obinary Debug/Exe/km0_image/km0_application.axf Debug/Exe/km0_image/xip_image2.bin

:: remove bss sections
%tooldir%\pick %ram2_start% %ram2_end% Debug\Exe\km0_image\ram_2.r.bin Debug\Exe\km0_image\ram_2.bin raw
del Debug\Exe\km0_image\ram_2.r.bin

:: add header
%tooldir%\pick %ram2_start% %ram2_end% Debug\Exe\km0_image\ram_2.bin Debug\Exe\km0_image\ram_2.p.bin
%tooldir%\pick %xip_image2_start% %xip_image2_end% Debug\Exe\km0_image\xip_image2.bin Debug\Exe\km0_image\xip_image2.p.bin

:: aggregate image2_all.bin
copy /b Debug\Exe\km0_image\xip_image2.p.bin+Debug\Exe\km0_image\ram_2.p.bin Debug\Exe\km0_image\km0_image2_all.bin
%tooldir%\pad Debug\Exe\km0_image\km0_image2_all.bin 4096

if exist Debug\Exe\km4_image\km4_image2_all.bin (
	copy /b Debug\Exe\km0_image\km0_image2_all.bin+Debug\Exe\km4_image\km4_image2_all.bin Debug\Exe\km0_image\km0_km4_image2.bin
) else (
	copy /b Debug\Exe\km0_image\km0_image2_all.bin+%libdir%\image\km4_image2_all.bin Debug\Exe\km0_image\km0_km4_image2.bin
)
copy Debug\Exe\km0_image\km0_km4_image2.bin Debug\Exe\km4_image\km0_km4_image2.bin

del Debug\Exe\km0_image\ram_2.bin
del Debug\Exe\km0_image\ram_2.p.bin
del Debug\Exe\km0_image\xip_image2.bin
del Debug\Exe\km0_image\xip_image2.p.bin

rename Debug\Exe\km0_image\km0_application.axf km0_application.dbg.axf

%iartooldir%\bin\ilinkarm.exe %tooldir%\link_dummy_lp.o --image_input Debug\Exe\km0_image\km0_km4_image2.bin,flash_start,firmware,32 --cpu Cortex-M23 --no_entry --keep flash_start --config rtl8721d_FLASH.icf --no_library_search --define_symbol __vector_table=0x00 -o Debug\Exe\km0_image\km0_application.axf

exit
