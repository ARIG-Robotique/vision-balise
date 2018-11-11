#ifndef VISION_BALISE_TESTER_H
#define VISION_BALISE_TESTER_H

#include "common.h"
#include "Config.h"
#include "Etallonage.h"
#include "Detection.h"


void runTest(Config *config) {
    Etallonage etalonnage(config);

    Mat source = imread("samples/DS1_1526.jpg", IMREAD_COLOR);
    etalonnage.run(source, 0);

    Detection detection(config);
    detection.run(source, 0);
}


#endif //VISION_BALISE_TESTER_H
