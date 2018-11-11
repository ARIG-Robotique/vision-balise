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

    fs["boardSize"] >> boardSize;
    fs["boardRatio"] >> boardRatio;
    fs["yCorrection"] >> yCorrection;
    fs["diametreObjet"] >> diametreObjet;
    fs["probeSize"] >> probeSize;

    FileNode markersPosNode = fs["markersPos"];
    readConfigVector<Point>(markersPos, markersPosNode);

    FileNode orangeRefsNode = fs["orangeRefs"];
    readConfigVector<Point>(orangeRefs, orangeRefsNode);

    FileNode greenRefsNode = fs["greenRefs"];
    readConfigVector<Point>(greenRefs, greenRefsNode);

    FileNode detectZonesNode = fs["detectZones"];
    readConfigVector<Rect>(detectZones, detectZonesNode);
    
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