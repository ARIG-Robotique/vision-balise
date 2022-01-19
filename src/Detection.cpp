#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

void to_json(json &j, const Echantillon &e) {
    j = json{
            {"c", e.c},
            {"x", e.x},
            {"y", e.y},
    };
}

json Detection::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("DETECTION {}", ++index);

    Mat projected;
    warpPerspective(source, projected, config->perspectiveMap, config->perspectiveSize);

    auto output = projected.clone();
    json r;

    vector<Echantillon> echantillons;
    vector<string> distribs = {STATUS_ABSENT, STATUS_ABSENT};
    detectMarkers(projected, output, echantillons, distribs);

    r["echantillons"] = echantillons;
    r["distribs"] = distribs;

    imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);

    spdlog::debug(r.dump(2));
    spdlog::debug("Détection en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

void
Detection::detectMarkers(const Mat &source, Mat &output, vector<Echantillon> &echantillons, vector<string> &distribs) {

    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners, rejectedCandidates;
    auto dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    auto parameters = aruco::DetectorParameters::create();
    aruco::detectMarkers(source, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

    aruco::drawDetectedMarkers(output, markerCorners, markerIds);
    aruco::drawDetectedMarkers(output, rejectedCandidates);

    auto zoneTopLeft = arig_utils::tablePtToImagePt(config->detectionZone.br());
    auto zoneBottomRight = arig_utils::tablePtToImagePt(config->detectionZone.tl());
    rectangle(output, Rect(zoneTopLeft, zoneBottomRight), arig_utils::GREEN, 2);

    zoneTopLeft = arig_utils::tablePtToImagePt(config->detectionZone2.br());
    zoneBottomRight = arig_utils::tablePtToImagePt(config->detectionZone2.tl());
    rectangle(output, Rect(zoneTopLeft, zoneBottomRight), arig_utils::GREEN, 2);

    for (auto i = 0; i < markerIds.size(); i++) {
        auto marker = markerCorners.at(i);

        string c;
        if (markerIds.at(i) == 47) {
            c = COLOR_RED;
        } else if (markerIds.at(i) == 13) {
            c = COLOR_BLUE;
        } else if (markerIds.at(i) == 36) {
            c = COLOR_GREEN;
        } else if (markerIds.at(i) == 17) {
            c = COLOR_ROCK;
        }

        if (!c.empty()) {
            auto pt = arig_utils::imagePtToTablePt(Point(
                    (marker.at(0).x + marker.at(2).x) / 2.0,
                    (marker.at(0).y + marker.at(2).y) / 2.0
            ));
            if (config->detectionZone.contains(pt)) {
                echantillons.emplace_back(Echantillon{
                        .c = c,
                        .x =pt.x,
                        .y =pt.y
                });
            } else if (config->detectionZone2.contains(pt) && c == COLOR_ROCK) {
                if (pt.x < 1500) {
                    distribs[0] = STATUS_PRESENT;
                } else {
                    distribs[1] = STATUS_PRESENT;
                }
            }
        }
    }
}