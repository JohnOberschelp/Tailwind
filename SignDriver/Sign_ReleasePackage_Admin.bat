
cd "%~dp0"

rmdir /S /Q DebugPackage
rmdir /S /Q ReleasePackage

mkdir ReleasePackage

copy tailwind.inf           ReleasePackage\tailwind.inf
copy ..\BuildDriver\x64\Release\tailwind.sys   ReleasePackage\tailwind.sys

"C:\Program Files (x86)\Windows Kits\10\bin\x86\Inf2Cat"    /driver:.\ReleasePackage /os:10_X64

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\Signtool" sign /v /fd sha256 /s PrivateCertStore /n Contoso.com(Test)        .\ReleasePackage\tailwind.sys

pause

exit

