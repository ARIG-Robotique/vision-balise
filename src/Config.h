#ifndef VISION_BALISE_CONFIG_H
#define VISION_BALISE_CONFIG_H

#include "common.h"


class Config {

public:
    bool debug = false;
    bool testMode = false;
    String outputDir;
    String outputPrefix;
    String mockPhoto;

    // from calibration
    Mat cameraMatrix;
    Mat distCoeffs;

    // from config
    int cameraIndex;
    Size cameraResolution;
    bool swapRgb;
    bool undistort;
    int markerId;
    int probeSize;
    int colorThreshold;

    // from API
    vector<Point> ecueil;
    vector<Point> bouees;

    // from etalonnage
    bool etalonnageDone = false;
    vector<Scalar> colorsEcueil;
    vector<Scalar> colorsBouees;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);

};


#endif //VISION_BALISE_CONFIG_H
