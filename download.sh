#!/usr/bin/env bash

ROOT_DIR=$(pwd)
DOWNLOAD_DIR=${ROOT_DIR}/download

OPENCV_VERSION=3.3.1
OPENCV_FILENAME=opencv-${OPENCV_VERSION}
OPENCV_DOWNLOAD_URL=https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip

JSON_VERSION=v2.1.1
JSON_DIR=json-${JSON_VERSION}
JSON_FILENAME=json.hpp
JSON_DOWNLOAD_URL=https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/${JSON_FILENAME}

RASPBERRY_TOOLS=https://github.com/raspberrypi/tools.git

echo "-- Download external dependencies"
if [ ! -d "${DOWNLOAD_DIR}" ] ; then
    echo "---- Make download directory : ${DOWNLOAD_DIR}"
    mkdir -p ${DOWNLOAD_DIR}
fi

cd ${DOWNLOAD_DIR}
if [ ! -f "${OPENCV_FILENAME}.zip" ] ; then
  echo "---- Download OpenCV ${OPENCV_VERSION} ..."
  curl -L ${OPENCV_DOWNLOAD_URL} -o ${OPENCV_FILENAME}.zip
  unzip ${OPENCV_FILENAME}.zip
  ln -s opencv-${OPENCV_VERSION} opencv
  mkdir opencv/build
  cd opencv/build
  cmake ..
  make
fi

cd $DOWNLOAD_DIR
if [ ! -f "${JSON_DIR}/${JSON_FILENAME}" ] ; then
    echo "---- Download JSON ${JSON_VERSION} ..."
    mkdir -p ${JSON_DIR} json
    curl -L ${JSON_DOWNLOAD_URL} -o ${JSON_DIR}/${JSON_FILENAME}
    cd json
    ln -sf ../${JSON_DIR}/${JSON_FILENAME}
fi

cd ${DOWNLOAD_DIR}
if [ ! -d "tools" ] ; then
    echo "---- Clone raspberry-tools ..."
    git clone --depth=1 ${RASPBERRY_TOOLS}
fi
