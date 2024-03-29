#ifndef VISION_BALISE_CONFIG_H
#define VISION_BALISE_CONFIG_H

#include "common.h"


class Config {

public:
    // from cli
    bool debug = false;
    bool testMode = false;
    String outputPrefix;
    String mockPhoto;

    // from calibration
    Mat cameraMatrix;
    Mat distCoeffs;
    Mat cameraK;
    Mat cameraD;
    Mat remap1, remap2;

    // from config
    int cameraIndex;
    Size cameraResolution;
    bool fisheye;
    bool swapRgb;
    bool undistort;
    int probeSize;
    int idleDelay;
    int detectionDelay;

    int seuilCluster;
    Rect detectionZone;
    Rect zoneDistribViolet;
    Rect zoneDistribJaune;

    // from etalonnage
    bool etalonnageDone = false;
    String team = TEAM_UNKNOWN;
    Mat perspectiveMap;
    Size perspectiveSize;

    Mat distribViolet;
    Mat distribJaune;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);

    void reset();

};

#endif //VISION_BALISE_CONFIG_H
