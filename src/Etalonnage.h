#ifndef VISION_BALISE_ETALONNAGE_H
#define VISION_BALISE_ETALONNAGE_H

#include "common.h"
#include "Config.h"

class Etalonnage {

private:
    Config* config;

public:
    explicit Etalonnage(Config* config);
    json run(const Mat &source);

};


#endif //VISION_BALISE_ETALONNAGE_H
