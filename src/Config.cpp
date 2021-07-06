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

// H +- 10
// S +- 75
// V +- 100
vector<Scalar> Config::getRedRange() const {
    auto hMin = red[0] - 10;
    if (hMin < 0) {
        hMin += 180;
    }
    auto hMax = red[0] + 10;
    if (hMax > 180) {
        hMax -= 180;
    }
    return {
            Scalar(hMin, max(red[1] - 75, 0.0), max(red[2] - 100, 0.0)),
            Scalar(hMax, min(red[1] + 75, 255.0), min(red[2] + 100, 255.0)),
    };
}

// H +- 10
// S +- 75
// V +- 50
vector<Scalar> Config::getGreenRange() const {
    return {
            Scalar(green[0] - 10, max(green[1] - 75, 0.0), max(green[2] - 100, 0.0)),
            Scalar(green[0] + 10, min(green[1] + 75, 255.0), min(green[2] + 100, 255.0))
    };
}

vector<Point> Config::getDetectionZone() const {
    return {
            Point(cameraResolution.width / 2 - 150, 100),
            Point(cameraResolution.width / 2 + 150, 100),
            Point(cameraResolution.width - 50, cameraResolution.height - 250),
            Point(50, cameraResolution.height - 250),
    };
}

void Config::reset() {
    etalonnageDone = false;
    team = TEAM_UNKNOWN;
    red = Scalar();
    green = Scalar();
    redEcueil = Scalar();
    greenEcueil = Scalar();
}
