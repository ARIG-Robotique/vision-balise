#ifndef VISION_BALISE_CALIBRATION_H
#define VISION_BALISE_CALIBRATION_H

#include "common.h"
#include "Config.h"

class Calibration {

public:
    bool runAndSave(const String &directory, const String &filename, const Config* config);

private:
    bool run(Mat &cameraMatrix, Mat &distCoeffs, const vector<String> &files, const Config* config);

};


#endif //VISION_BALISE_CALIBRATION_H
