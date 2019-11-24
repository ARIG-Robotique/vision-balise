#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

json Detection::run(const Mat &source, int index) {
    spdlog::info("Detection {}", index);

    json r;

    // correction de l'objectif
    Mat undistorted;
    undistort(source, undistorted, config->cameraMatrix, config->distCoeffs);

    // TODO

    spdlog::debug("DETECTION RESULT\n {}", r.dump(2));

    return r;
}