:: Formats specific parts of the library.

@ECHO OFF
IF NOT EXIST ..\src (
    ECHO Can not find source directory
    EXIT
)

cd ..\src

ECHO Formatting all the source file's in the src directory

for /r %%t in (*.cpp *.c *.h) do clang-format -i --style=file "%%t"

cd ..\scripts