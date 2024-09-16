
cd /d "%~dp0"

"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\windbg" -k net:port=50000,key=1.j.a.4 -c "$$>< tailwindWinDbgStartupScript.wdb"

