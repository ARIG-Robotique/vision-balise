#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

json Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE");

    Mat output = source.clone();
    json r;

    config->colorsEcueil = readColors(source, config->ecueil);
    r["ecueil"] = arig_utils::scalars2json(config->colorsEcueil);
    drawProbes(output, config->ecueil, config->colorsEcueil);

    if (!config->bouees.empty()) {
        config->colorsBouees = readColors(source, config->bouees);
        r["bouees"] = arig_utils::scalars2json(config->colorsBouees);
        drawProbes(output, config->bouees, config->colorsBouees);
    } else {
        config->colorsBouees.clear();
    }

    imwrite(config->outputPrefix + "etallonage.jpg", output);

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

void Etalonnage::drawProbes(const Mat &output, vector<Point> &points, vector<Scalar> &colors) {
    auto white = Scalar(255, 255, 255);

    for (auto i = 0; i < points.size(); i++) {
        Rect probe = arig_utils::getProbe(points[i], config->probeSize);
        Rect display = arig_utils::getProbe(points[i] - Point(40, 40), config->probeSize);

        rectangle(output, probe.tl(), probe.br(), white);
        line(output, probe.tl(), display.br(), white);
        rectangle(output, display.tl(), display.br(), arig_utils::ScalarHSV2BGR(colors[i]), FILLED);
        rectangle(output, display.tl(), display.br(), white);
    }
}
