@cd /d "%~dp0"
@rem set NRFUTIL="%ProgramFiles(x86)%\Nordic Semiconductor\Master Control Panel\3.10.0.14\nrf\nrfutil.exe"
@set NRFUTIL="nrfutil.exe"
"%NRFUTIL%" dfu genpkg Update.zip --application ".\BLE_App\ble_app_uart_s130_pca10028 THUMB Release\ble_app_uart_s130_pca10028.hex" --application-version 0 --dev-revision 0xB001 --dev-type 0xB001 --sd-req 0x80
@pause
