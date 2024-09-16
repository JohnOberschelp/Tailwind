# Steps to Set Up a Target Machine

**This is how I set up my Windows 10 Target machine to test Tailwind.**

(Surely not the best way, but worked for me.)

1. **Reinstall windows**.  
   
   - Windows \> Settings \> Update & security \> Recovery \> Reset this PC \> remove Everything (4 GB download; approximately 1 hour)

2. **Change drive letters if necessary**; some scripts still assume the drive will be E:
   
   - File Explorer \> This PC > Manage  
   - Then from Computer Management \> Storage \> Disk Management \> E: More Actions

3. **Set up a shared folder on the Target**, for convenience.
   
   - Both PCs were configured Sharing \> Manage Advanced sharing \> private \> Turn ON net discovery + Turn on Automatic setup private \> file & printer sharing \> Turn ON file and printer sharing
   - Guest or Public (current profile) \> Network discovery \> Turn ON
   - Guest or Public (current profile) \> File & Printer sharing \> Turn ON
   - All Networks \> Public folder sharing \> Turn ON
   - File sharing connections \> Use 128-bit
   - Password protected sharing \> Turn OFF
   - Create C:\\share and set it as a shared folder
   - Open File Explorer on the Client computer
   - Type "\\\\TARGET_COMPUTER_NAME_HERE\\share" at the top and drag that address to the desktop.

4. **Set up the Target the way you like**.
   
   - I change power config, screen saver, background, mouse, show hidden, show extensions, \...
   - I sometimes install 7-zip, SciTE, Firefox as default, grepWin, WinMerge, HxD, \...

5. **Copy over the driver and testing files**.
   
   - Copy over a Package folder, format.exe, AddVlcAndPlayBbb1TimeOnE.bat and Firefox Portable 91.0.1.

6. **Plug in the thumb drive**.
   
   - Shut down the Target, plug in the thumb drive, then power back up.
   - For some scripts, the thumb drive is expected to be E:

7. **Install the driver**.
   
   - Right-click alpha.inf, and choose install. Reboot.

8. **Start the driver**.
   
   - To start with an empty drive, run the command "format.exe E:"
   - Run as admin, sc_start_alpha_ADMIN.bat

9. **Test the driver**.
   
   - Example 1: Run the AddVlcAndPlay1TimeOnE.bat batch file.
   - Example 2: Drag a copy of Firefox Portable_91.0.1_English.paf.exe to E:, Launch the installer, refresh to see the folder, launch the Portable Firefox, and browse.

10. **Set up KDNET network kernel debugging**.
    
    - (see Microsoft's notes [here](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-a-network-debugging-connection-automatically).)
    - Record the ip address of both the Client and the Target computers.
    - Copy Client files...  
      `C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\*kdnet.exe*`  
      ... and...  
      `C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\VerifiedNICList.xml`  
      ... to Target `C:\\KDNET`  
    - On the Target as admin, run...  
      cd C:\\KDNET  
      kdnet.exe
    - (The NIC must be supported.)
    - kdnet.exe \<ClientComputerIPAddress\> 50000
    - Save the connection key generated to use on the Client.

11. **Run WinDbg**.
    
    - On the Client, execute...  
      cd C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64
    - windbg.exe -k net:port=50000,key=actual.key.value.here
    - In WinDbg, to get all debug output, do...  
      ed nt!Kd_DEFAULT_MASK 0xF  
      or, better, to set it permanently, update the registry with...  
      (in Admin prompt on Target)  
      C:\\\> reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session  
      Manager\\\Debug Print Filter\" /v DEFAULT /t REG_DWORD /d 0xf
