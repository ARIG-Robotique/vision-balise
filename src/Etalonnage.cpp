#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

JsonResult Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE {}", ++index);

    Mat output = source.clone();
    JsonResult r;

    imwrite(config->outputPrefix + "etallonage-result-" + to_string(index) + ".jpg", output);

    r.status = RESPONSE_OK;
    r.data = arig_utils::matToBase64(output);

    config->etalonnageDone = true;

    spdlog::debug("Etalonnage en {}ms", arig_utils::ellapsedTime(start));

    return r;
}
