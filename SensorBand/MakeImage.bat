@cd /d "%~dp0"
@if exist "OutputImage.hex" del "OutputImage.hex"
@if exist "Merged_S130_and_DFU.hex" del "Merged_S130_and_DFU.hex"
@if exist "Merged_S130_and_DFU_and_App.hex" del "Merged_S130_and_DFU_and_App.hex"
@xcopy /y /d "BLE_BootApp\Output\THUMB Release\Exe\*.hex" .
@xcopy /y /d "BLE_BootApp\S130_OTA_DFU THUMB Release\*.hex" .
@xcopy /y /d "BLE_App\Output\THUMB Release\Exe\*.hex" .
@xcopy /y /d "BLE_App\ble_app_uart_s130_pca10028 THUMB Release\*.hex" .
@rem set MERGEHEX=%ProgramFiles(x86)%\Nordic Semiconductor\nrf5x\bin\mergehex.exe
@set MERGEHEX=mergehex.exe
@
"%MERGEHEX%" --merge ".\nRF5_SDK_11.0.0_89a8197\components\softdevice\s130\hex\s130_nrf51_2.0.0_softdevice.hex" "S130_OTA_DFU.hex" --output "Merged_S130_and_DFU.hex"
@
"%MERGEHEX%" --merge "Merged_S130_and_DFU.hex" "ble_app_uart_s130_pca10028.hex" --output "Merged_S130_and_DFU_and_App.hex"
@
"%MERGEHEX%" --merge "Merged_S130_and_DFU_and_App.hex" ".\Common\Validate_DFU_nvm_settings.hex" --output "OutputImage.hex"
@
@pause
