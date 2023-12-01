@echo off

cl.exe main.cpp /nologo /EHsc /Fe:tameicons.exe /link shell32.lib user32.lib ole32.lib oleaut32.lib

del main.obj
