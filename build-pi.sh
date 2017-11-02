#!/usr/bin/env bash
# Author : Gregory DEPUILLE
# Description : Wrapper pour la configuration et le build du projet
PROJECT="ARIG C++ Vision balise (cross compile Raspberry PI)"

echo "Build du projet $PROJECT"

ROOT_DIR=$(pwd)
BUILD_DIR=$ROOT_DIR/build-pi

if [ -d "$BUILD_DIR" ] ; then
    echo "-- Nettoyage du répertoire de build $BUILD_DIR"
    rm -Rf $BUILD_DIR
fi

echo "-- Création du répertoire de build $BUILD_DIR"
mkdir -p $BUILD_DIR

echo "-- Build du projet $PROJECT"
export PI_TOOLS_HOME=$(pwd)/download/tools

sh download.sh

cd $BUILD_DIR
cmake -DCMAKE_TOOLCHAIN_FILE=raspberry.cmake .. || exit $?
cmake --build . || exit $?
echo "Build terminé"
