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
    detectMarkers(projected, output, echantillons);
    detectDistribs(projected, output, distribs);

    r["echantillons"] = echantillons;
    r["distribs"] = distribs;

    if (config->debug || index % 2 == 0) {
        imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);
    }

    spdlog::debug(r.dump(2));
    spdlog::debug("Détection en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

void Detection::detectDistribs(const Mat &source, Mat &output, vector<string> &distribs) {

    auto zoneViolet = arig_utils::tableRectToImageRect(config->zoneDistribViolet);
    auto zoneJaune = arig_utils::tableRectToImageRect(config->zoneDistribJaune);
    rectangle(output, zoneViolet, arig_utils::GREEN, 2);
    rectangle(output, zoneJaune, arig_utils::GREEN, 2);

    Mat extractedViolet = source(zoneViolet);
    Mat extractedJaune = source(zoneJaune);

    auto normViolet = norm(config->distribViolet, extractedViolet);
    auto normJaune = norm(config->distribJaune, extractedJaune);
    auto similarityViolet = 1 - normViolet / (zoneViolet.width * zoneViolet.height);
    auto similarityJaune = 1 - normJaune / (zoneJaune.width * zoneJaune.height);

    spdlog::debug("Similarity distrib violet : {}", similarityViolet);
    spdlog::debug("Similarity distrib jaune : {}", similarityJaune);

    if (similarityViolet > 0) {
        distribs[0] = STATUS_PRESENT;
    }
    if (similarityJaune > 0) {
        distribs[1] = STATUS_PRESENT;
    }
}

void Detection::detectMarkers(const Mat &source, Mat &output, vector<Echantillon> &echantillons) {

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
                        .x = pt.x,
                        .y = pt.y
                });
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
        auto l = 25;

        if (abs(l - d1) <= d && abs(l - d2) <= d && abs(l - d3) <= d && abs(l - d4) <= d) {
            auto center = arig_utils::markerCenter(marker);
            auto pt = arig_utils::imagePtToTablePt(center);
            if (config->detectionZone.contains(pt)) {
                echantillons.emplace_back(Echantillon{
                        .c = COLOR_UNKNOWN,
                        .x = pt.x,
                        .y = pt.y
                });
                putText(output, "UNKN", center, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
            }
        }
    }
}