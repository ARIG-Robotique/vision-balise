#ifndef VISION_BALISE_CONFIG_H
#define VISION_BALISE_CONFIG_H

#include "common.h"


class Config {

public:
    bool debug = false;
    bool testMode = false;
    String outputDir;
    String outputPrefix;

    // from calibration
    Mat cameraMatrix;
    Mat distCoeffs;

    // from config
    int cameraIndex;
    Size boardSize;
    float boardRatio;
    int yCorrection;
    int diametreObjet;
    int probeSize;
    vector<Point> markersPos;
    vector<Point> orangeRefs;
    vector<Point> greenRefs;
    vector<Rect> detectZones;

    // runtime
    Mat homography;
    Scalar orange;
    Scalar green;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);
    bool readEtallonageFile(const String &filename);

};


#endif //VISION_BALISE_CONFIG_H
