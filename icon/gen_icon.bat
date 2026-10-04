@REM @echo off

set "icon=sinc2"

magick identify "%icon%.png"
magick "%icon%.png" -background transparent -define icon:auto-resize=256,128,64,48,32,16 "%icon%.ico"
magick identify "%icon%.ico"

echo IDI_ICON1 ICON "%icon%.ico" > "icon.rc"

windres "icon.rc" -O coff -o "%icon%.res"

pause
