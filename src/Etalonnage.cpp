#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

JsonResult Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE {}", ++index);

    auto output = source.clone();
    JsonResult r;

    Point markerCenter;
    vector<Point> markersEchantillons;
    if (!detectMarkers(source, output, markerCenter, markersEchantillons)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de trouver les marqueurs";
        return r;
    }

    vector<vector<Point>> clusters;
    getClusters(output, markersEchantillons, clusters);

    vector<Point2f> ptsImages;
    vector<Point2f> ptsTable;
    if (!detectPerspectivePoints(clusters, ptsImages, ptsTable)) {
        imwrite(config->outputPrefix + "/etallonage-result-" + to_string(index) + ".jpg", output);
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de calculer la perspective";
        return r;
    }

    config->perspectiveMap = getPerspectiveTransform(ptsImages, ptsTable);
    config->perspectiveSize = Size(1500, 1000);
    config->etalonnageDone = true;

    Mat projected;
    warpPerspective(source, projected, config->perspectiveMap, config->perspectiveSize);

    config->distribViolet = projected(arig_utils::tableRectToImageRect(config->zoneDistribViolet));
    config->distribJaune = projected(arig_utils::tableRectToImageRect(config->zoneDistribJaune));

    r.status = RESPONSE_OK;
    r.data = arig_utils::matToBase64(projected);

    imwrite(config->outputPrefix + "/etallonage-result-" + to_string(index) + ".jpg", output);

    spdlog::debug("Etalonnage en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

bool Etalonnage::detectMarkers(const Mat &source, Mat &output, Point &markerCenter, vector<Point> &markersEchantillons) {

    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners, rejectedCandidates;
    auto dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    auto parameters = aruco::DetectorParameters::create();
    aruco::detectMarkers(source, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

    aruco::drawDetectedMarkers(output, markerCorners, markerIds);
    aruco::drawDetectedMarkers(output, rejectedCandidates);

    for (auto i = 0; i < markerIds.size(); i++) {
        auto pt = arig_utils::markerCenter(markerCorners.at(i));
        if (markerIds.at(i) == 42) {
            markerCenter.x = pt.x;
            markerCenter.y = pt.y;
        } else if (markerIds.at(i) == 17) {
            markersEchantillons.push_back(pt);
        }
    }

    if (markerCenter.x == 0 && markerCenter.y == 0) {
        spdlog::warn("Impossible de trouver le marqueur central");
        return false;
    }

    if (markersEchantillons.empty()) {
        spdlog::warn("Impossible de trouver les marqueurs échantillons");
        return false;
    }

    return true;
}

void Etalonnage::getClusters(Mat &output, const vector<Point> &markers, vector<vector<Point>> &clusters) {
    for (const auto &marker : markers) {
        bool markerClustered = false;
        for (auto &cluster : clusters) {
            for (const auto &otherMarker : cluster) {
                if (cv::norm(otherMarker - marker) < config->seuilCluster) {
                    cluster.push_back(marker);
                    markerClustered = true;
                    break;
                }
            }
            if (markerClustered) {
                break;
            }
        }

        if (!markerClustered) {
            vector<Point> newCluster;
            newCluster.push_back(marker);
            clusters.push_back(newCluster);
        }
    }

    for (int i = 0; i < clusters.size(); i++) {
        for (auto const &marker : clusters.at(i)) {
            putText(output, to_string(i), marker, 0, 0.5, Scalar(0, 0, 0), 3);
            putText(output, to_string(i), marker, 0, 0.5, Scalar(0, 0, 255), 1);
        }
    }
}

bool Etalonnage::detectPerspectivePoints(const vector<vector<Point>> &clusters, vector<Point2f> &ptsImages, vector<Point2f> &ptsTable) {
    bool foundZoneViolet = false, foundZoneJaune = false;

    for (const auto &cluster : clusters) {
        if (cluster.size() == 3) { // un des "triangles"
            if (cluster.at(0).x < config->cameraResolution.width / 2.0) { // coté violet
                if (foundZoneViolet) {
                    spdlog::warn("Deux zones de fouille violettes trouvées");
                    return false;
                }
                foundZoneViolet = true;

                auto top = arig_utils::pointsOfMinY(cluster);
                auto bottom = arig_utils::pointsOfMaxY(cluster);

                ptsImages.push_back(top.at(0));
                ptsImages.push_back(bottom.at(0));

                ptsTable.push_back(arig_utils::tablePtToImagePt(Point(2100, 795)));
                ptsTable.push_back(arig_utils::tablePtToImagePt(Point(2100, 555)));

            } else { // coté jaune
                if (foundZoneJaune) {
                    spdlog::warn("Deux zones de fouille jaunes trouvées");
                    return false;
                }
                foundZoneJaune = true;

                auto top = arig_utils::pointsOfMinY(cluster);
                auto bottom = arig_utils::pointsOfMaxY(cluster);

                ptsImages.push_back(top.at(0));
                ptsImages.push_back(bottom.at(0));

                ptsTable.push_back(arig_utils::tablePtToImagePt(Point(900, 795)));
                ptsTable.push_back(arig_utils::tablePtToImagePt(Point(900, 555)));
            }
        }
    }

    if (!foundZoneJaune || !foundZoneViolet) {
        spdlog::warn("Toutes les zones n'ont pas été trouvées");
        return false;
    }

    return true;
}