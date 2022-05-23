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

    if (config->debug || index % 2 == 0) {
        imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);
    }

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
            auto pt = arig_utils::imagePtToTablePt(arig_utils::markerCenter(marker));
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

    // récupère les markers non identifiés (ceux qui sont bien carrés, mais sans ids)
    for (const auto &marker: rejectedCandidates) {
        if (marker.size() != 4) {
            continue;
        }
        auto d1 = norm(marker.at(0) - marker.at(1));
        auto d2 = norm(marker.at(1) - marker.at(2));
        auto d3 = norm(marker.at(2) - marker.at(3));
        auto d4 = norm(marker.at(3) - marker.at(0));
        auto d = 5;

        if (abs(d1 - d2) <= d && abs(d2 - d3) <= d && abs(d3 - d4) <= d && abs(d4 - d1) <= d) {
            auto pt = arig_utils::imagePtToTablePt(arig_utils::markerCenter(marker));
            if (config->detectionZone.contains(pt)) {
                echantillons.emplace_back(Echantillon{
                        .c = COLOR_UNKNOWN,
                        .x =pt.x,
                        .y =pt.y
                });
                putText(output, "UNKN", marker.at(0), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
            }
        }
    }
}