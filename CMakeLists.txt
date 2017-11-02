#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   02/11/2017                                                          #
# Description: Build du programme de vision balise                            #
#=============================================================================#
# Contrôle de la version minimale de CMake
cmake_minimum_required(VERSION 3.2.2)

# Check program requis
function (checkProgram name)
    message("-- Detecting ${name} program")
    find_program(PROGRAM_PRESENT ${name})
    if (NOT PROGRAM_PRESENT)
        message(FATAL_ERROR "${name} must be installed")
    else()
        message("-- Detecting ${name} program - done")
    endif()
endfunction(checkProgram)

# Variable pour les dépendances externes
set(DOWNLOAD_DIR "download")

set(OpenCV_DIR        "${DOWNLOAD_DIR}/opencv/build")
set(JSON_LIB_DIR      "${DOWNLOAD_DIR}/json")

# Variable de build
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
find_package(OpenCV REQUIRED)
#find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

include_directories(${OpenCV_INCLUDE_DIRS})

# Récupération de la définition pour JSON #
###########################################
include_directories(${JSON_LIB_DIR})

# Récupération des sources du projet principale #
#################################################
file(GLOB SRCS "src/*.cpp")
file(GLOB HEADERS "src/*.h")

add_executable(${PROJECT_NAME} ${SRCS} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} pthread)
