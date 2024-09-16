
cd "%~dp0"

rmdir /S /Q DebugPackage
rmdir /S /Q ReleasePackage

mkdir DebugPackage

copy tailwind.inf         DebugPackage\tailwind.inf
copy ..\BuildDriver\x64\Debug\tailwind.sys   DebugPackage\tailwind.sys

"C:\Program Files (x86)\Windows Kits\10\bin\x86\Inf2Cat"    /driver:.\DebugPackage /os:10_X64

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\Signtool" sign /v /fd sha256 /s PrivateCertStore /n Contoso.com(Test)        .\DebugPackage\tailwind.sys

pause

exit

