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
    Size cameraResolution;
    Size boardSize;
    bool swapRgb;
    float boardRatio;
    int yCorrection;
    int diametreObjet;
    int probeSize;
    vector<Point> markersPos;
    vector<Point> blueRefs;
    vector<Point> redRefs;
    vector<Point> greenRefs;
    vector<Rect> detectZones;

    // runtime
    Mat homography;
    Scalar blue;
    Scalar red;
    Scalar green;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);
    bool readEtallonageFile(const String &filename);

};


#endif //VISION_BALISE_CONFIG_H
