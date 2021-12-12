#ifndef VISION_BALISE_TEST_H
#define VISION_BALISE_TEST_H

#include "common.h"
#include "utils.h"
#include "Config.h"

void show(Mat &mat, bool ok) {
    if (ok) {
        imshow("debug", mat);
        waitKey(0);
    }
}

void runTest(Mat &source, const Config *config) {
    bool debugAll = false;

    show(source, debugAll);

    // correction objectif
    Mat undistorted;
    remap(source, undistorted, config->remap1, config->remap2, INTER_LINEAR);

    show(undistorted, debugAll);

    Mat work = undistorted.clone();

    show(work, true);
}

#endif //VISION_BALISE_TEST_H
