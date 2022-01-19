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
    fs["probeSize"] >> probeSize;
    fs["idleDelay"] >> idleDelay;
    fs["detectionDelay"] >> detectionDelay;

    fs["seuilCluster"] >> seuilCluster;
    fs["detectionZone"] >> detectionZone;
    fs["detectionZone2"] >> detectionZone2;

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

        Mat newCameraMatrix;
        fisheye::estimateNewCameraMatrixForUndistortRectify(cameraK, cameraD, cameraResolution,
                                                            Matx33d::eye(), newCameraMatrix, 0.0);
        fisheye::initUndistortRectifyMap(cameraK, cameraD,
                                         Matx33d::eye(), newCameraMatrix, cameraResolution,
                                         CV_16SC2, remap1, remap2);

    } else {
        fs["cameraMatrix"] >> cameraMatrix;
        fs["distCoeffs"] >> distCoeffs;

        Mat newCameraMatrix = getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, cameraResolution, 0.0);
        initUndistortRectifyMap(cameraMatrix, distCoeffs,
                                Matx33d::eye(), newCameraMatrix, cameraResolution,
                                CV_16SC2, remap1, remap2);
    }

    return true;
}

void Config::reset() {
    etalonnageDone = false;
    team = TEAM_UNKNOWN;
}
