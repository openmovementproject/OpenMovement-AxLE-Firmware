# OpenMovement-AxLE-Firmware

OpenMovement AxLE device firmware source code.

This is tested with *Rowley CrossWorks CrossStudio for ARM* (free trial available), main project file `SensorBand/BLE_App/ble_app_uart_s130_pca10028.hzp`.

This has not been tested, but may work with [Segger Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/), which is free for *Nordic SDK users*:

1. Unzip `nRF5_SDK_11.0.0_89a8197.zip`, preserving any existing files in `nRF5_SDK_11.0.0_89a8197` (most of these changes are not required, but it will make the folder the same as the original build).
2. Install [Embedded Studio for ARM](https://www.segger.com/downloads/embedded-studio) (at time of writing, V4.30b 2019-11-19)
3. *Tools* / *Package Manager* / install Nordic Semiconductor's *nRF CPU Support Package*
4. *File* / *Open Solution* / `SensorBand/BLE_App/ble_app_uart_s130_pca10028.hzp`
5. Select the build configuration `THUMB Release`:
  * Under *System Files*, right-click `APP_section_placement_with_DFU+S130.xml` and select *Use This Section Placement File*.
  * Under *System Files*, right-click `nRF51822_xxAA_MemoryMap.xml` and select *Use This Memory Map File*.
6. *Build* / *Build Solution*.
7. Repeat from step 3 for `SensorBand/BLE_BootApp/OTA_Bootloader.hzp`.  You will need to replace `nRF51_Startup.s` with `../Common/nRF51_Startup.s`, and add `../Common/nRF51822_xxAA_MemoryMap.xml` to the *System Files* folder.
8. Run `MakeImage.bat` to generate `OutputImage.hex`.
9. Run `MakeDFU.bat` to generate `update.zip`.

<!--
rev 8097
*.hzp "../../" -> ""
-->
