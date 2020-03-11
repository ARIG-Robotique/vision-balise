#!/usr/bin/env bash
# Author : Gregory DEPUILLE
# Description : Wrapper pour la configuration et le build du projet
PROJECT="ARIG C++ Vision balise (cross compile Raspberry PI GCC 6.5.0)"

echo "Build du projet ${PROJECT}"

ROOT_DIR=$(pwd)
BUILD_NAME=build-pi-new
BUILD_DIR=${ROOT_DIR}/${BUILD_NAME}

cd ${ROOT_DIR}
if [ -d "${BUILD_DIR}" ] ; then
    echo "-- Nettoyage du répertoire de build ${BUILD_DIR}"
    rm -Rf ${BUILD_DIR}
fi

echo "-- Création du répertoire de build ${BUILD_DIR}"
mkdir -p ${BUILD_DIR}

sh download.sh ${BUILD_NAME}

echo "-- Build OpenCV"
cd ${ROOT_DIR}/download/opencv/${BUILD_NAME}
cmake --build . || exit $?

echo "-- Build du projet ${PROJECT}"
cd ${BUILD_DIR}
cmake -DBUILD_PI_NEW=true .. || exit $?
cmake --build . || exit $?
