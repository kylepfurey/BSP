@echo off
cd extern
git clone https://github.com/microsoft/vcpkg.git "%USERPROFILE%\vcpkg"
call "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat"
"%USERPROFILE%\vcpkg\vcpkg.exe" install sdl3          ^
                                        nlohmann-json
