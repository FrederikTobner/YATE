# Run the Editor
echo "Configuring CMake ..."
cmake -B ../build -S .. --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DCMAKE_BUILD_TYPE=Debug -G Ninja -DCMAKE_C_COMPILER=/usr/local/bin/clang-16 ..
echo "Building Editor ..."
cmake --build ../build --config Debug
if [ -d "../build/src" ]
then
    echo "Executing Editor ..."
    ../build/src/yate
else
    echo "No Source build folder generated ..."
    exit 70
fi
