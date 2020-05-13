# Element de config :
# http://amgaera.github.io/blog/2014/04/10/cross-compiling-for-raspberry-pi-on-64-bit-linux/
# https://medium.com/@au42/the-useful-raspberrypi-cross-compile-guide-ea56054de187

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(XC_DIR x64-gcc-6.5.0/arm-rpi-linux-gnueabihf)

# specify the cross compiler
set(CMAKE_C_COMPILER ${PI_TOOLS_HOME}/${XC_DIR}/bin/arm-rpi-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${PI_TOOLS_HOME}/${XC_DIR}/bin/arm-rpi-linux-gnueabihf-g++)

# where is the target environment
set(CMAKE_FIND_ROOT_PATH ${PI_TOOLS_HOME}/${XC_DIR}/arm-rpi-linux-gnueabihf/sysroot)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
