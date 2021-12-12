#ifndef VISION_BALISE_ETALONNAGE_H
#define VISION_BALISE_ETALONNAGE_H

#include "common.h"
#include "Config.h"

class Etalonnage {

private:
    Config *config;

    int index = 0;

public:
    explicit Etalonnage(Config *config);
    JsonResult run(const Mat &source);

private:

};

#endif //VISION_BALISE_ETALONNAGE_H
