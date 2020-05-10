#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

json Detection::run(const Mat &source, int index) {
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
            r["bouees"] = readColors(source, config->bouees);
        }
    }

    spdlog::debug(r.dump(2));

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
    int dX = config->ecueil[1].x - config->ecueil[0].x;
    int dY = config->ecueil[1].y - config->ecueil[0].y;

    vector<Point> points;

    for (unsigned short i = 0; i < 5; i++) {
        Point pt = Point(config->ecueil[0].x + dX / 5.0 * i, config->ecueil[0].y + dY / 5.0 * i);
        points.emplace_back(pt);
    }

    return readColors(image, points);
}

/**
 * Lecture des couleurs d'un ensemble de points
 */
vector<string> Detection::readColors(const Mat &image, const vector<Point> &points) {
    vector<string> colors;

    for (auto &pt : points) {
        Scalar color = arig_utils::getAverageColor(image, arig_utils::getProbe(pt, config->probeSize));
        int hue = arig_utils::ScalarBGR2HSV(color)[0];

        int dRed = abs(hue - config->colorsEcueil[1][0]);
        dRed = min(dRed, 180 - dRed);
        int dGreen = abs(hue - config->colorsEcueil[0][0]);
        dGreen = min(dGreen, 180 - dGreen);

        if (dRed < config->colorThreshold) {
            colors.emplace_back(COLOR_RED);
        } else if (dGreen < config->colorThreshold) {
            colors.emplace_back(COLOR_GREEN);
        } else {
            colors.emplace_back(COLOR_UNKNOWN);
        }
    }

    return colors;
}