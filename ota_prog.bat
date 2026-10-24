@ECHO OFF
REM OTA-program RFID boxes: ota_prog.bat [box numbers...]
REM   ota_prog.bat          all boxes 1-5
REM   ota_prog.bat 3        only box3
REM   ota_prog.bat 1 4      box1 and box4
REM Runs ota_upload.sh with Git Bash.
bash "%~dp0ota_upload.sh" %*
