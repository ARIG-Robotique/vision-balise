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

    Mat output = source.clone();
    json r;

    vector<Echantillon> echantillons;
    detectMarkers(source, output, echantillons);

    // TODO
    vector<string> distribs;

    r["echantillons"] = echantillons;
    r["distribs"] = distribs;

    imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);

    spdlog::debug(r.dump(2));
    spdlog::debug("Détection en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

void Detection::detectMarkers(const Mat &source, Mat &output, vector<Echantillon> &result) {

    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    aruco::detectMarkers(source, dictionary, markerCorners, markerIds);

    aruco::drawDetectedMarkers(output, markerCorners, markerIds);

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
            Point pt = arig_utils::imagePtToTablePt(Point(
                    (marker.at(0).x + marker.at(2).x) / 2.0,
                    (marker.at(0).x + marker.at(2).x) / 2.0
            ));
            result.emplace_back(Echantillon{
                    .c =  c,
                    .x= pt.x,
                    .y=pt.y
            });
        }
    }
}