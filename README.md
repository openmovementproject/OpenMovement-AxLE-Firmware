# OpenMovement-AxLE-Firmware

OpenMovement AxLE device firmware source code.

This is tested with *Rowley CrossWorks CrossStudio for ARM* (free trial available), main project file `SensorBand/BLE_App/ble_app_uart_s130_pca10028.hzp`.

This has not been tested, but may work with [Segger Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/), which is free for *Nordic SDK users*:

1. Install [Embedded Studio for ARM](https://www.segger.com/downloads/embedded-studio) (at time of writing, V4.30b 2019-11-19)
2. *Tools* / *Package Manager* / install Nordic Semiconductor's *nRF CPU Support Package*
3. *File* / *Open Solution* / `SensorBand/BLE_App/ble_app_uart_s130_pca10028.hzp`
4. Select the build configuration `THUMB Release`:
  * Under *System Files*, right-click `APP_section_placement_with_DFU+S130.xml` and select *Use This Section Placement File*.
  * Under *System Files*, right-click `nRF51822_xxAA_MemoryMap.xml` and select *Use This Memory Map File*.
5. *Build* / *Build Solution*.
6. Repeat from step 3 for `SensorBand/BLE_BootApp/OTA_Bootloader.hzp`.  You will need to replace `nRF51_Startup.s` with `../Common/nRF51_Startup.s`, and add `../Common/nRF51822_xxAA_MemoryMap.xml` to the *System Files* folder.
7. Run `MakeImage.bat` to generate `OutputImage.hex`.
8. Run `MakeDFU.bat` to generate `update.zip`.

<!--
rev 8097
*.hzp "../../" -> ""
-->
