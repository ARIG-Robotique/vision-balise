#include "Config.h"

template<class T>
void readConfigVector(vector<T> &array, FileNode &node) {
    FileNodeIterator it = node.begin(), end = node.end();

    array.resize(node.size());

    for (int idx = 0; it != end; ++it, idx++) {
        *it >> array[idx];
    }
}

bool Config::readConfigFile(const String &filename) {
    FileStorage fs(filename, FileStorage::READ);

    if (!fs.isOpened()) {
        return false;
    }

    fs["cameraIndex"] >> cameraIndex;
    fs["cameraResolution"] >> cameraResolution;
    fs["swapRgb"] >> swapRgb;

    return true;
}

bool Config::readCalibrationFile(const String &filename) {
    FileStorage fs(filename, FileStorage::READ);

    if (!fs.isOpened()) {
        return false;
    }

    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;

    return true;
}
