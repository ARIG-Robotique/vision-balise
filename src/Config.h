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
    int colorThreshold;
    int detectionBuffer;
    int detectionValidLimit;
    int idleDelay;
    int detectionDelay;

    // from etalonnage
    bool etalonnageDone = false;
    String team = TEAM_UNKNOWN;
    Scalar red, green;
    Scalar redEcueil, greenEcueil;
    Mat perspectiveMap;
    Size perspectiveSize;

    // from detection
    String girouette = DIR_UNKNOWN;

    vector<Scalar> getRedRange() const;
    vector<Scalar> getGreenRange() const;
    vector<Point> getDetectionZone() const;

    bool readConfigFile(const String &filename);
    bool readCalibrationFile(const String &filename);

    void reset();

};


#endif //VISION_BALISE_CONFIG_H
