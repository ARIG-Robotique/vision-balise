SET (CMAKE_SYSTEM_NAME Linux)
SET (CMAKE_SYSTEM_VERSION 1)

# Check for Raspberry Pi Tools and bail out if they don't have it
if(NOT DEFINED ENV{PI_TOOLS_HOME})
    message(FATAL_ERROR "Raspberry PI build need 'export PI_TOOLS_HOME=<path>' environment variable")
    return()
endif()

# Element de config :
# http://amgaera.github.io/blog/2014/04/10/cross-compiling-for-raspberry-pi-on-64-bit-linux/

SET (PI_TOOLS_DIR arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf)

# specify the cross compiler
SET (CMAKE_C_COMPILER $ENV{PI_TOOLS_HOME}/${PI_TOOLS_DIR}/bin/arm-linux-gnueabihf-gcc)
SET (CMAKE_CXX_COMPILER $ENV{PI_TOOLS_HOME}/${PI_TOOLS_DIR}/bin/arm-linux-gnueabihf-g++)

# where is the target environment
SET (CMAKE_FIND_ROOT_PATH $ENV{PI_TOOLS_HOME}/${PI_TOOLS_DIR})

# search for programs in the build host directories
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)