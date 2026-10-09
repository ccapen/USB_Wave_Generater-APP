#!/bin/bash

main_exe="./USB Wave Generater.exe"
libserialport="C:/msys64/mingw64/bin/libserialport-0.dll"

rm -rf "./release/USB_Wave_Generater-APP/"
mkdir -p "./release/USB_Wave_Generater-APP/"

ldd "${main_exe}" | grep -v '/c/windows' | awk '{print $3}' | sort -u | xargs -I{} cp -n {} "./release/USB_Wave_Generater-APP/"

cp "${main_exe}"	"./release/USB_Wave_Generater-APP/${main_exe}"
cp "${libserialport}" "./release/USB_Wave_Generater-APP/libserialport-0.dll"
