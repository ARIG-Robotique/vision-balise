#ifndef VISION_BALISE_TEST_H
#define VISION_BALISE_TEST_H

#include "common.h"
#include "utils.h"
#include "Config.h"

void show(Mat &mat, bool ok) {
    if (ok) {
        imshow("debug", mat);
        waitKey(0);
    }
}

void runTest(Mat &source, const Config *config) {
    bool debugAll = false;

    show(source, debugAll);

    // correction objectif
    Mat undistorted;
    remap(source, undistorted, config->remap1, config->remap2, INTER_LINEAR);

    show(undistorted, debugAll);

    // détection des marqueurs
    Mat output = undistorted.clone();

    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    aruco::detectMarkers(undistorted, dictionary, markerCorners, markerIds);
    aruco::drawDetectedMarkers(output, markerCorners, markerIds);

    show(output, debugAll);

    // on cherche le marqueur 42 et les marqueurs 17
    vector<Point> marker42;
    vector<Point> markers17;

    for (auto i = 0; i < markerIds.size(); i++) {
        auto marker = markerCorners.at(i);
        if (markerIds.at(i) == 42) {
            marker42.emplace_back(Point(
                    (marker.at(0).x + marker.at(1).x) / 2.0,
                    (marker.at(0).y + marker.at(1).y) / 2.0
            ));
        } else if (markerIds.at(i) == 17) {
            markers17.emplace_back(Point(
                    (marker.at(0).x + marker.at(1).x) / 2.0,
                    (marker.at(0).y + marker.at(1).y) / 2.0
            ));
        }
    }

    if (marker42.size() != 1 || markers17.empty()) {
        spdlog::warn("Impossible de trouver les marqueurs");
        return;
    }

    int seuilDistance = 100;

    // créée des groupes de marqueurs 17
    vector<vector<Point>> clusters;
    for (const auto &marker : markers17) {
        bool markerClustered = false;
        for (auto &cluster : clusters) {
            for (const auto &otherMarker : cluster) {
                if (cv::norm(otherMarker - marker) < seuilDistance) {
                    cluster.emplace_back(marker);
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
            newCluster.emplace_back(marker);
            clusters.emplace_back(newCluster);
        }
    }

    Mat work = undistorted.clone();

    for (int i = 0; i < clusters.size(); i++) {
        for (auto const &marker : clusters.at(i)) {
            circle(work, marker, 5, Scalar(0, 0, 255), 1);
            putText(work, to_string(i), marker + Point(10, 10), 0, 0.5, Scalar(0, 0, 0), 1);
        }
    }

    show(work, debugAll);

    // détermine la correspondance de chaque marqueur
    vector<Point> ptsImages;
    vector<Point> ptsTable;

    ptsImages.emplace_back(marker42.at(0));
    ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(1500, 1250)));

    bool foundAbriViolet, foundAbriJaune, foundZoneViolet, foundZoneJaune;

    for (const auto &cluster : clusters) {
        if (cluster.size() == 3) { // un des "triangles"
            if (cluster.at(0).x < config->cameraResolution.width / 2) { // coté violet
                if (foundZoneViolet) {
                    spdlog::warn("Deux zones de fouille violet trouvée");
                    return;
                }
                foundZoneViolet = true;

                auto top = arig_utils::pointsOfMinY(cluster);
                auto middle = arig_utils::pointsOfMinX(cluster);
                auto bottom = arig_utils::pointsOfMaxY(cluster);

                ptsImages.emplace_back(top.at(0));
                ptsImages.emplace_back(middle.at(0));
                ptsImages.emplace_back(bottom.at(0));

                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(2100, 795)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(2170, 675)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(2100, 555)));

            } else { // coté jaune
                if (foundZoneJaune) {
                    spdlog::warn("Deux zones de fouille jaune trouvée");
                    return;
                }
                foundZoneJaune = true;

                auto top = arig_utils::pointsOfMinY(cluster);
                auto middle = arig_utils::pointsOfMaxX(cluster);
                auto bottom = arig_utils::pointsOfMaxY(cluster);

                ptsImages.emplace_back(top.at(0));
                ptsImages.emplace_back(middle.at(0));
                ptsImages.emplace_back(bottom.at(0));

                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(900, 795)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(830, 675)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(900, 555)));
            }

        } else if (cluster.size() == 2) {
            if (cluster.at(0).x < config->cameraResolution.width / 2) { // coté violet
                if (foundAbriViolet) {
                    spdlog::warn("Deux abris de chantier violet trouvé");
                    return;
                }
                foundAbriViolet = true;

                auto gauche = arig_utils::pointsOfMinX(cluster);
                auto droite = arig_utils::pointsOfMaxX(cluster);

                ptsImages.emplace_back(gauche.at(0));
                ptsImages.emplace_back(droite.at(0));

                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(2879, 1688)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(2688, 1879)));

            } else { // coté jaune
                if (foundAbriJaune) {
                    spdlog::warn("Deux abris de chantier jaune trouvé");
                    return;
                }
                foundAbriJaune = true;

                auto gauche = arig_utils::pointsOfMinX(cluster);
                auto droite = arig_utils::pointsOfMaxX(cluster);

                ptsImages.emplace_back(gauche.at(0));
                ptsImages.emplace_back(droite.at(0));

                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(312, 1879)));
                ptsTable.emplace_back(arig_utils::tablePtToImagePt(Point(121, 1688)));
            }
        }
    }

    if (!foundAbriJaune || !foundAbriViolet || !foundZoneJaune || !foundZoneViolet) {
        spdlog::warn("Toutes les zones n'ont pas été trouvées");
        return;
    }

    // calcule la perspective
    Mat proj = getPerspectiveTransform(ptsImages, ptsTable);

    Mat projected;
    warpPerspective(undistorted, projected, proj, Size(1500, 1100));

    show(projected, debugAll);
}

#endif //VISION_BALISE_TEST_H
