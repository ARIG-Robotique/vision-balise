#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

json Detection::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("DETECTION {}", ++index);

    Mat output = source.clone();
    json r;

    imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);

    spdlog::debug(r.dump(2));
    spdlog::debug("Détection en {}ms", arig_utils::ellapsedTime(start));

    return r;
}
