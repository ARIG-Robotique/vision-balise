#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

json Etalonnage::run(const Mat &source) {
    spdlog::info("Etalonnage");

    Scalar red = arig_utils::getAverageColor(source, arig_utils::getProbe(config->redPoint, config->probeSize));
    Scalar green = arig_utils::getAverageColor(source, arig_utils::getProbe(config->greenPoint, config->probeSize));

    config->red = arig_utils::ScalarBGR2HSV(red);
    config->green = arig_utils::ScalarBGR2HSV(green);
    config->etalonnageDone = true;

    spdlog::debug("COLORS CALIBRATION\n red: {}\n green: {}", config->red, config->green);

    return json({
            {"done",  true},
            {"red",   arig_utils::scalar2json(config->red)},
            {"green", arig_utils::scalar2json(config->green)}
    });
}