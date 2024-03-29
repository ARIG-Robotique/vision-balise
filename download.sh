#!/usr/bin/env bash

ROOT_DIR=$(pwd)
DOWNLOAD_DIR=${ROOT_DIR}/download

OPENCV_VERSION=3.4.5
OPENCV_FILENAME=opencv-${OPENCV_VERSION}
OPENCV_DOWNLOAD_URL=https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip

OPENCV_CONTRIB_FILENAME=opencv_contrib-${OPENCV_VERSION}
OPENCV_CONTRIB_DOWNLOAD_URL=https://github.com/opencv/opencv_contrib/archive/${OPENCV_VERSION}.zip
OPENCV_CONTRIB_MODULES=aruco

JSON_VERSION=v3.10.5
JSON_DIR=json-${JSON_VERSION}
JSON_FILENAME=json.hpp
JSON_DOWNLOAD_URL=https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/${JSON_FILENAME}

SPDLOG_VERSION=1.3.1
SPDLOG_FILENAME=spdlog-${SPDLOG_VERSION}
SPDLOG_DOWNLOAD_URL=https://github.com/gabime/spdlog/archive/v${SPDLOG_VERSION}.zip

SSD1306_VERSION=master
SSD1306_FILENAME=libSSD1306-${SSD1306_VERSION}
SSD1306_DOWNLOAD_URL=https://github.com/AndrewFromMelbourne/libSSD1306/archive/${SSD1306_VERSION}.zip

RASPBERRY_TOOLS=https://github.com/rvagg/rpi-newer-crosstools.git

echo "-- Download external dependencies"
if [ ! -d "${DOWNLOAD_DIR}" ] ; then
    echo "---- Make download directory : ${DOWNLOAD_DIR}"
    mkdir -p ${DOWNLOAD_DIR}
fi

cd ${DOWNLOAD_DIR}
if [ ! -d "tools" ] ; then
    echo "---- Clone raspberry-tools ..."
    git clone --depth=1 ${RASPBERRY_TOOLS} tools
fi

cd ${DOWNLOAD_DIR}
if [ ! -f "${OPENCV_CONTRIB_FILENAME}.zip" ] ; then
  echo "---- Download OpenCV Contrib ${OPENCV_VERSION} ..."
  curl -L ${OPENCV_CONTRIB_DOWNLOAD_URL} -o ${OPENCV_CONTRIB_FILENAME}.zip
  unzip ${OPENCV_CONTRIB_FILENAME}.zip

  mkdir -p opencv_contrib/modules
fi

cd ${DOWNLOAD_DIR}
for i in $(echo ${OPENCV_CONTRIB_MODULES} | sed "s/,/ /g")
do
    if [ ! -d "opencv_contrib/modules/${i}" ] ; then
        echo "---- Link OpenCV Contrib ${i}"
        ln -s ../../${OPENCV_CONTRIB_FILENAME}/modules/${i} opencv_contrib/modules/${i}
    fi
done

cd ${DOWNLOAD_DIR}
if [ ! -f "${OPENCV_FILENAME}.zip" ] ; then
  echo "---- Download OpenCV ${OPENCV_VERSION} ..."
  curl -L ${OPENCV_DOWNLOAD_URL} -o ${OPENCV_FILENAME}.zip
  unzip ${OPENCV_FILENAME}.zip
  ln -s ${OPENCV_FILENAME} opencv
fi

if [ "${1}" = "build-pi" ] ; then
    echo "-- Config pour PI"
    cd ${DOWNLOAD_DIR}
    if [ ! -d "opencv/build-pi" ] ; then
        mkdir opencv/build-pi
    fi
    cd opencv/build-pi
    cmake \
      -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
      -DPI_TOOLS_HOME=${DOWNLOAD_DIR}/tools -DCMAKE_TOOLCHAIN_FILE=${DOWNLOAD_DIR}/../raspberry.cmake \
      -DBUILD_SHARED_LIBS=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DBUILD_JAVA=OFF \
      ..
else
    echo "-- Config pour PC local"
    cd ${DOWNLOAD_DIR}
    if [ ! -d "opencv/build" ] ; then
        mkdir opencv/build
    fi
    cd opencv/build
    cmake \
      -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
      -DBUILD_SHARED_LIBS=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DBUILD_JAVA=OFF \
      ..
fi

cd ${DOWNLOAD_DIR}
if [ ! -f "${JSON_DIR}/${JSON_FILENAME}" ] ; then
    echo "---- Download JSON ${JSON_VERSION} ..."
    mkdir -p ${JSON_DIR} json
    curl -L ${JSON_DOWNLOAD_URL} -o ${JSON_DIR}/${JSON_FILENAME}
    cd json
    ln -sf ../${JSON_DIR}/${JSON_FILENAME}
fi

cd ${DOWNLOAD_DIR}
if [ ! -f "${SPDLOG_FILENAME}.zip" ] ; then
  echo "---- Download SPDLOG ${SPDLOG_VERSION} ..."
  curl -L ${SPDLOG_DOWNLOAD_URL} -o ${SPDLOG_FILENAME}.zip
  unzip ${SPDLOG_FILENAME}.zip
  ln -s ${SPDLOG_FILENAME}/include spdlog
fi

cd ${DOWNLOAD_DIR}
if [ ! -f "${SSD1306_FILENAME}.zip" ] ; then
  echo "--- Download SSD1306 ${SSD1306_VERSION} ..."
  curl -L ${SSD1306_DOWNLOAD_URL} -o ${SSD1306_FILENAME}.zip
  unzip ${SSD1306_FILENAME}.zip
  ln -s ${SSD1306_FILENAME} libSSD1306
fi
