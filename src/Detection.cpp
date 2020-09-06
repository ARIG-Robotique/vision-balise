#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;

    for (auto i = 0; i < 5; i++) {
        ecueilBuffer.emplace_back(vector<int>(config->detectionBuffer, 0));
    }
    for (auto i = 0; i < config->bouees.size(); i++) {
        boueesBuffer.emplace_back(vector<int>(config->detectionBuffer, 0));
    }
}

json Detection::run(const Mat &source, int index) {
    auto start = arig_utils::startTiming();
    spdlog::info("DETECTION {}", index);

    json r;

    // recherche des marqueurs
    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    findMarkers(source, markerCorners, markerIds);

    if (config->debug) {
        Mat output = source.clone();
        aruco::drawDetectedMarkers(output, markerCorners, markerIds);
        imwrite(config->outputPrefix + "detection-markers-" + to_string(index) + ".jpg", output);
    }

    vector<Point2f> marker = getMarkerById(markerCorners, markerIds, config->markerId);

    if (marker.empty()) {
        spdlog::info("Marker not found");
        r["direction"] = DIR_UNKNOWN;
    } else {
        bool upside = isMarkerUpside(marker);
        r["direction"] = upside ? DIR_UP : DIR_DOWN;
    }

    // lecture des couleurs
    if (config->etalonnageDone) {
        r["ecueil"] = readColorsEcueil(source);

        if (!config->bouees.empty()) {
            r["bouees"] = checkPresenceBouees(source);
        }

        bufferIndex++;
        if (bufferIndex == config->detectionBuffer) {
            bufferIndex = 0;
        }
    }

    spdlog::debug(r.dump(2));
    spdlog::debug("Done in {}ms", arig_utils::ellapsedTime(start));

    return r;
}

/**
 * Trouve les marqueurs ARUCO sur l'image
 * @param image
 * @param markerCorners
 * @param markerIds
 */
void Detection::findMarkers(const Mat &image, vector<vector<Point2f>> &markerCorners, vector<int> &markerIds) {
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);

    aruco::detectMarkers(image, dictionary, markerCorners, markerIds);

    if (spdlog::default_logger()->level() == spdlog::level::debug) {
        stringstream ss;
        ss << "MARKERS RESULT" << endl;
        ss << "MarkersIds: ";
        for (auto const &id : markerIds) ss << to_string(id) << ",";
        ss << endl;
        ss << "MarkersPos: ";
        for (auto const &marker : markerCorners) {
            ss << "(";
            for (auto const &pos : marker) ss << pos << ",";
            ss << ") ";
        }

        spdlog::debug(ss.str());
    }
}

/**
 * Retourne un marqueur par son id
 * @param markerCorners
 * @param markerIds
 * @param id
 * @return
 */
vector<Point2f> Detection::getMarkerById(vector<vector<Point2f>> &markerCorners, vector<int> &markerIds, int id) {
    for (unsigned long i = 0; i < markerIds.size(); i++) {
        if (markerIds.at(i) == id) {
            return markerCorners.at(i);
        }
    }

    return vector<Point2f>();
}

/**
 * Vérifie si un marqueur est plutot orienté vers le haut de l'image
 * Evo possible  utiliser estimatePoseSingleMarkers
 * @param marker
 * @return
 */
bool Detection::isMarkerUpside(vector<Point2f> &marker) {
    // 0         1
    //  +-------+
    //  |       |
    //  |       |
    //  +-------+
    // 3         2
    float y0 = marker.at(0).y;
    float y3 = marker.at(3).y;
    float x0 = marker.at(0).x;
    float x1 = marker.at(1).x;
    return y0 < y3 && x0 < x1;
}

/**
 * Lecture des cinq couleurs de l'ecueil
 */
vector<string> Detection::readColorsEcueil(const Mat &image) {
    vector<string> colors;

    auto dX = config->ecueil[1].x - config->ecueil[0].x;
    auto dY = config->ecueil[1].y - config->ecueil[0].y;

    for (auto i = 0; i < 5; i++) {
        auto pt = Point(config->ecueil[0].x + dX / 4.0 * i, config->ecueil[0].y + dY / 4.0 * i);
        auto color = arig_utils::getAverageColor(image, arig_utils::getProbe(pt, config->probeSize));
        auto hue = arig_utils::ScalarBGR2HSV(color)[0];

        auto dRed = abs(hue - config->colorsEcueil[1][0]);
        dRed = min(dRed, 180 - dRed);
        auto dGreen = abs(hue - config->colorsEcueil[0][0]);
        dGreen = min(dGreen, 180 - dGreen);

        if (dRed < config->colorThreshold) {
            ecueilBuffer[i][bufferIndex] = 1;
        } else if (dGreen < config->colorThreshold) {
            ecueilBuffer[i][bufferIndex] = -1;
        } else {
            ecueilBuffer[i][bufferIndex] = 0;
        }

        auto val = 0.0;
        for (auto j = 0; j < config->detectionBuffer; j++) {
            val += ecueilBuffer[i][j];
        }

        if (val >= config->detectionValidLimit) {
            colors.emplace_back(COLOR_RED);
        } else if (val <= -config->detectionValidLimit) {
            colors.emplace_back(COLOR_GREEN);
        } else {
            colors.emplace_back(COLOR_UNKNOWN);
        }
    }

    return colors;
}

/**
 * Vérification de présence de chaque bouée
 */
vector<string> Detection::checkPresenceBouees(const Mat &image) {
    vector<string> result;

    for (auto i = 0; i < config->bouees.size(); i++) {
        auto pt = config->bouees[i];
        auto color = arig_utils::getAverageColor(image, arig_utils::getProbe(pt, config->probeSize));

        auto d = abs(arig_utils::ScalarBGR2HSV(color)[0] - config->colorsBouees[i][0]);
        d = min(d, 180 - d);

        if (d < config->colorThreshold) {
            boueesBuffer[i][bufferIndex] = 1;
        } else {
            boueesBuffer[i][bufferIndex] = 0;
        }

        auto val = 0.0;
        for (auto j = 0; j < config->detectionBuffer; j++) {
            val += boueesBuffer[i][j];
        }

        if (val >= config->detectionValidLimit) {
            result.emplace_back(BOUE_PRESENT);
        } else {
            result.emplace_back(BOUE_ABSENT);
        }
    }

    return result;
}