#!/usr/bin/env bash
# Author : Gregory DEPUILLE
# Description : Wrapper pour la configuration et le build du projet
PROJECT="ARIG C++ Vision balise (cross compile Raspberry PI GCC 6.3.1)"

echo "Build du projet $PROJECT"

ROOT_DIR=$(pwd)
BUILD_NAME=build-pi-new
BUILD_DIR=${ROOT_DIR}/${BUILD_NAME}

cd ${ROOT_DIR}
echo "Build du projet ${PROJECT}"
if [ -d "${BUILD_DIR}" ] ; then
    echo "-- Nettoyage du répertoire de build ${BUILD_DIR}"
    rm -Rf ${BUILD_DIR}
fi

echo "-- Création du répertoire de build ${BUILD_DIR}"
mkdir -p ${BUILD_DIR}

sh download.sh ${BUILD_NAME}

echo "-- Configuration du projet ${PROJECT}"
cd ${BUILD_DIR}
cmake -DPI_TOOLS_HOME=${ROOT_DIR}/download/newer-tools -DBUILD_PI_NEW=true .. || exit $?

echo "-- Build OpenCV"
cd ${ROOT_DIR}/download/opencv/${BUILD_NAME}
cmake --build . || exit $?

echo "-- Build du projet ${PROJECT}"
cd ${BUILD_DIR}
cmake --build . || exit $?
echo "Build terminé"
