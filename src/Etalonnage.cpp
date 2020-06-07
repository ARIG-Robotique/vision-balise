#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

json Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE");

    json r;

    config->colorsEcueil = readColors(source, config->ecueil);
    r["ecueil"] = arig_utils::scalars2json(config->colorsEcueil);

    if (!config->bouees.empty()) {
        config->colorsBouees = readColors(source, config->bouees);
        r["bouees"] = arig_utils::scalars2json(config->colorsBouees);
    } else {
        config->colorsBouees.clear();
    }

    config->etalonnageDone = true;

    spdlog::debug(r.dump(2));
    spdlog::debug("Done in {}ms", arig_utils::ellapsedTime(start));

    return r;
}

vector<Scalar> Etalonnage::readColors(const Mat &source, vector<Point> &points) {
    vector<Scalar> colors;

    for (auto &pt : points) {
        Scalar color = arig_utils::getAverageColor(source, arig_utils::getProbe(pt, config->probeSize));
        colors.emplace_back(arig_utils::ScalarBGR2HSV(color));
    }

    return colors;
}
