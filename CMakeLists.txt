cmake_minimum_required(VERSION 3.0.0)
project(testSpioSpi)

set(LIB_SRC
    SdioSpi/SdioSpi.c
    SdioSpi/SdioSpi.h
)

set(LIB_PATH
    SdioSpi
)

set(TEST_SRC
    main.c
)

include_directories(
    ${LIB_PATH}
)

add_executable(testSdioSpi.exe ${LIB_SRC} ${TEST_SRC})