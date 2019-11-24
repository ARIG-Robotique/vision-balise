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
    bool swapRgb;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);

};


#endif //VISION_BALISE_CONFIG_H
