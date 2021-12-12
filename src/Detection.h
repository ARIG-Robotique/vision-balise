#ifndef VISION_BALISE_DETECTION_H
#define VISION_BALISE_DETECTION_H

#include "common.h"
#include "Config.h"

class Detection {

private:
    Config* config;

    int index = 0;

public:
    explicit Detection(Config* config);
    json run(const Mat &source);

private:

};

#endif //VISION_BALISE_DETECTION_H
