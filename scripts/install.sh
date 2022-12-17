# Builds Editor in release mode
echo "Configuring CMake ..."
cmake -B ../build -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_C_COMPILER=clang-16 ..
echo "Building YATE ..."
cmake --build ../build --config Release
echo "Installing YATE ..."
sudo cmake --install ../build/src --config Release