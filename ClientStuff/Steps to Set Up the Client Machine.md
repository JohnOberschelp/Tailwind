# Steps to Set Up the Client Machine

**This is how I set up my Windows 10 Client machine to develop Tailwind.**

Note: Set up the Target computer first. The Client setup requires the KDNET key generated while setting up the Target machine.

1. **Set up a shared folder on the Target**, for convenience.
2. **Build**.
   - Build tailwind.sys, Format.exe and, optionally, Chat.exe
   - Increment the DriverVer (driver version) in tailwind.inf 
   - Run as admin, Sign_DebugPackage_Admin.bat
3. **Copy over files**.
   - Copy over the DebugPackage folder created, Format.exe, and, optionally, Chat.exe
4. **Set up WinDbg**.
   - Edit tailwindWinDbg.bat to use the KDNET key generated while setting up the Target machine.
   - Edit tailwindWinDbgStartupScript.wdb to use the your Client machine paths.
   - Start WinDbg with tailwindWinDbg.bat
