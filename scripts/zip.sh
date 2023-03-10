# Creates zipped executable of the Editor for x64
# Configuring CMake
echo "Configuring CMake for x64 ..."
cmake -B ../build/linux_x64 -DCMAKE_BUILD_TYPE=Release -G Ninja  -DCMAKE_C_COMPILER=/usr/local/bin/clang-16 ..
echo "Building Editor for x64 ..."
cmake --build ../build/linux_x64 --config Release --target Cellox
# Zipping binaries
echo "Zipping YATE x64 ..."
if [ -d "../build/linux_x64/src" ] 
then
    cd ../build/linux_x64/src
    cpack -G ZIP --config ../CPackConfig.cmake.    
else
    echo "Can not find x64 linux build folder ..."
    exit 70
fi