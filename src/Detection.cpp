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

    // recherche des marqueurs
    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    findMarkers(undistorted, markerCorners, markerIds);

    vector<Point2f> marker = getMarkerById(markerCorners, markerIds, config->markerId);

    if (marker.empty()) {
        spdlog::info("Marker not found");
        r["direction"] = "UNKNOWN";
    } else {
        bool upside = isMarkerUpside(marker);
        r["direction"] = upside ? "UP" : "DOWN";
    }

    spdlog::debug("DETECTION RESULT\n {}", r.dump(2));

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
 * Evo possible  uriliser estimatePoseSingleMarkers
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
    return y0 > y3 && x0 < x1;
}
