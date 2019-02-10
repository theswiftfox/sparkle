@echo off
msbuild /m /p:Platform=x64 /p:Configuration=Release %~dp0\3rdParty\ChakraCore\Build\Chakra.Core.sln
pause