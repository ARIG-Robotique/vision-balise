#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   02/11/2017                                                          #
# Description: Build du programme de vision balise                            #
#=============================================================================#
# Contrôle de la version minimale de CMake
cmake_minimum_required(VERSION 3.9.2)

# Check program requis
function (checkProgram name)
    message(STATUS "Detecting ${name} program")
    find_program(PROGRAM_PRESENT ${name})
    if (NOT PROGRAM_PRESENT)
        message(FATAL_ERROR "${name} must be installed")
    else()
        message(STATUS "Detecting ${name} program - done")
    endif()
endfunction(checkProgram)

# Variable pour les dépendances externes
set(DOWNLOAD_DIR "download")

# Pour faire de la cross compilation
if (DEFINED BUILD_PI)
    set(PI_TOOLS_HOME ${CMAKE_CURRENT_SOURCE_DIR}/${DOWNLOAD_DIR}/tools)
    set(CMAKE_TOOLCHAIN_FILE ./raspberry.cmake)
endif()

message(STATUS "Build PI     : ${BUILD_PI}")

# Variable de build
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOT DEFINED ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug mode ON")
    add_definitions(-DDEBUG_MODE)
endif()

# Check program
checkProgram(git)
checkProgram(curl)
checkProgram(unzip)

#====================================================================#
#  Setup Project                                                     #
#====================================================================#
project(vision_balise)

# Récupération des dépendances par le script de download
execute_process(
        COMMAND sh download.sh
        WORKING_DIRECTORY ..
)

# Inclusion de la lib externe OpenCV #
########################################
set(OpenCV_SHARED OFF)
if (DEFINED ${BUILD_PI})
    set(OpenCV_DIR "${DOWNLOAD_DIR}/opencv/build-pi")
else()
    set(OpenCV_DIR "${DOWNLOAD_DIR}/opencv/build")
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui aruco)

message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

#include_directories(${OpenCV_INCLUDE_DIRS})

# Récupération de la définition pour JSON #
###########################################
set(JSON_LIB_DIR "${DOWNLOAD_DIR}/json")
include_directories(${JSON_LIB_DIR})

# Récupération de la définition pour SPDLOG #
#############################################
set(SPDLOG_LIB_DIR "${DOWNLOAD_DIR}/spdlog")
include_directories(${SPDLOG_LIB_DIR})

# Récupération de la définition pour libSSD1306 #
#################################################
set(SSD1306_DIR "${DOWNLOAD_DIR}/libSSD1306")
add_subdirectory(${SSD1306_DIR})
include_directories(${SSD1306_DIR}/lib)

# Récupération des sources du projet #
######################################
if (DEFINED BUILD_PI)
    add_compile_definitions(PI)
endif()

file(GLOB SRCS "src/*.cpp")
file(GLOB HEADERS "src/*.h")

add_executable(${PROJECT_NAME} ${SRCS} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} SSD1306 pthread)
