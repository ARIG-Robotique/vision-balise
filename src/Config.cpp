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
    fs["fisheye"] >> fisheye;
    fs["swapRgb"] >> swapRgb;
    fs["undistort"] >> undistort;
    fs["markerId"] >> markerId;
    fs["probeSize"] >> probeSize;
    fs["colorThreshold"] >> colorThreshold;
    fs["detectionBuffer"] >> detectionBuffer;
    fs["detectionValidLimit"] >> detectionValidLimit;
    fs["idleDelay"] >> idleDelay;
    fs["detectionDelay"] >> detectionDelay;

    return true;
}

bool Config::readCalibrationFile(const String &filename) {
    FileStorage fs(filename, FileStorage::READ);

    if (!fs.isOpened()) {
        return false;
    }

    if (fisheye) {
        fs["k"] >> cameraK;
        fs["d"] >> cameraD;
    } else {
        fs["cameraMatrix"] >> cameraMatrix;
        fs["distCoeffs"] >> distCoeffs;
    }

    return true;
}
