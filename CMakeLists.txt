project(Spectrogen)
cmake_minimum_required(VERSION 3.5)

set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set (CMAKE_C_STANDARD 11)

find_library(FFTW_LIBRARY NAMES fftw3 fftw)
set(FFTW_LIBRARIES "${FFTW_LIBRARY}")

# Do not remove the following auto-generated signatures

# Auto-generated. Do not edit. All changes will be undone
set(SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/main.c
   )
# Auto-generated end


add_executable(Spectrogen ${SOURCE_FILES})
target_link_libraries(Spectrogen m)
target_link_libraries(Spectrogen SDL2)
