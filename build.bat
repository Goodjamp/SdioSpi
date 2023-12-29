rmdir /S /Q build
cmake -DCMAKE_C_COMPILER:FILEPATH=arm-none-eabi-gcc.exe -DCMAKE_CXX_COMPILER:FILEPATH="arm-none-eabi-g++.exe" -DBUILD_MODE:STRING=%1 -B build  -G "MinGW Makefiles"
cd build
make -j16
cd ..
