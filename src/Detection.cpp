#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

json Detection::run(const Mat &source, int index) {
    spdlog::info("Detection {}", index);

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
        r["direction"] = "UNKNOWN";
    } else {
        bool upside = isMarkerUpside(marker);
        r["direction"] = upside ? "UP" : "DOWN";
    }

    // lecture des couleurs
    if (config->etalonnageDone) {
        vector<string> colors = readColors(source);
        r["colors"] = colors;
    }

    spdlog::debug("DETECTION {} RESULT\n {}", to_string(index), r.dump(2));

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
    for (unsigned short i = 0; i < markerIds.size(); i++) {
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
 * Lecture des trois couleurs entre bouées verte et rouge
 */
vector<string> Detection::readColors(const Mat &image) {
    vector<string> colors;

    int dX = config->redPoint.x - config->greenPoint.x;
    int dY = config->redPoint.y - config->greenPoint.y;

    for (unsigned int i = 1; i <= 3; i++) {
        Point pt = Point(config->greenPoint.x + dX / 5.0 * i, config->greenPoint.y + dY / 5.0 * i);
        Scalar color = arig_utils::getAverageColor(image, arig_utils::getProbe(pt, config->probeSize));
        int hue = arig_utils::ScalarBGR2HSV(color)[0];

        int dRed = abs(hue - config->red[0]);
        dRed = min(dRed, 180 - dRed);
        int dGreen = abs(hue - config->green[0]);
        dGreen = min(dGreen, 180 - dGreen);

        if (dRed < 42) {
            colors.emplace_back("RED");
        } else if (dGreen < 42) {
            colors.emplace_back("GREEN");
        } else {
            colors.emplace_back("UNKNOWN");
        }
    }

    return colors;
}