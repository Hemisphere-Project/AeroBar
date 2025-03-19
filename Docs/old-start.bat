taskkill /f /im "MadMapper.exe"
cd /D "%~dp0"
cd "midi2osc"
pm2 start "midi2osc.js" -f
cd /D "%~dp0"
echo F|xcopy /y "AeroBar-RISE-original.mad" "%userprofile%\Documents\RISE.mad"
start "" "C:\Program Files\MadMapper 5.6.6\MadMapper.exe" "AeroBar-RISE-original.mad"




taskkill /f /im "MadMapper.exe"
cd /D "%~dp0"
cd "midi2osc"
pm2 start "midi2osc.js" -f
cd /D "%~dp0"
echo F|xcopy /y "AeroBar-RISE-original.mad" "%userprofile%\Documents\RISE.mad"
"C:\Program Files\MadMapper 5.6.6\MadMapper.exe" "%userprofile%\Documents\RISE.mad"