cmake_minimum_required(VERSION 3.0.0)
project(testSpioSpi)

set(LIB_SRC
    SdSpi/SdSpi.c
    SdSpi/SdSpi.h
    SdSpi/SdSpiInternal.h
)

set(LIB_PATH
    SdSpi
)

set(TEST_SRC
    main.c
)

include_directories(
    ${LIB_PATH}
)

add_executable(${CMAKE_PROJECT_NAME} ${LIB_SRC} ${TEST_SRC})