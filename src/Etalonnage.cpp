#include "Etalonnage.h"
#include "Detection.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

JsonResult Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE {}", ++index);

    Mat output1 = source.clone();
    JsonResult r;

    // DETECTION DU MARKER
    Point markerCenter;
    if (!detectMarker(source, output1, markerCenter)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de trouver le marqueur";
        return r;
    }

    spdlog::debug("Marker 42 found : {}", markerCenter);

    config->team = markerCenter.x > config->cameraResolution.width / 2.0 ? TEAM_BLEU : TEAM_JAUNE;
    spdlog::debug("Équipe {}", config->team);


    // CALIBRATION DES COULEURS
    if (!calibCouleurs(source, output1, markerCenter)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de calibrer les couleurs";
        return r;
    }

    spdlog::debug("Rouge {}", config->red);
    spdlog::debug("Vert {}", config->green);


    // DETECTION DE BOUEES AUX EXTREMITES
    // zone de detection pour eviter les faux positifs
    vector<Point> detectionZone = config->getDetectionZone();
    vector<Scalar> redRange = config->getRedRange();
    vector<Scalar> greenRange = config->getGreenRange();

    Mat imageHsv;
    cvtColor(source, imageHsv, COLOR_BGR2HSV);

    Point bouee8, bouee12, bouee5, bouee9;
    if (!detectBouees(imageHsv, output1, redRange, detectionZone, false, bouee8, bouee12) ||
        !detectBouees(imageHsv, output1, greenRange, detectionZone, true, bouee9, bouee5)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de détecter les bouées";
        return r;
    }

    spdlog::debug("Bouee 8 : {}", bouee8);
    spdlog::debug("Bouee 12 : {}", bouee12);
    spdlog::debug("Bouee 9 : {}", bouee9);
    spdlog::debug("Bouee 5 : {}", bouee5);

    polylines(output1, detectionZone, true, arig_utils::BLUE, 1);
    circle(output1, bouee8, 20, arig_utils::RED, 1);
    circle(output1, bouee12, 20, arig_utils::RED, 1);
    circle(output1, bouee9, 20, arig_utils::GREEN, 1);
    circle(output1, bouee5, 20, arig_utils::GREEN, 1);

    imwrite(config->outputPrefix + "etallonage-" + to_string(index) + ".jpg", output1);


    // CALCUL PERSPECTIVE
    Point2f ptsImages[] = {bouee9, bouee8, bouee5, bouee12};
    Point2f ptsProj[] = {
            arig_utils::tablePtToImagePt(Point(1270, 1200)),
            arig_utils::tablePtToImagePt(Point(1730, 1200)),
            arig_utils::tablePtToImagePt(Point(2330, 100)),
            arig_utils::tablePtToImagePt(Point(670, 100)),
    };

    config->perspectiveMap = getPerspectiveTransform(ptsImages, ptsProj);
    config->perspectiveSize = Size(1500, 1100);

    Mat projected;
    warpPerspective(source, projected, config->perspectiveMap, config->perspectiveSize);

    Mat output2 = projected.clone();

    if (!calibCouleursEcueil(projected, output2)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de calibrer les couleurs de l'ecueil";
        return r;
    }

    spdlog::debug("Rouge ecueil {}", config->redEcueil);
    spdlog::debug("Vert ecueil {}", config->greenEcueil);

    debugResult(output2);
    imwrite(config->outputPrefix + "etallonage-result-" + to_string(index) + ".jpg", output2);

    r.status = RESPONSE_OK;
    r.data = arig_utils::matToBase64(output2);

    config->etalonnageDone = true;

    spdlog::debug("Etalonnage en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

/**
 * Applique la correction de perspective et écrit un fichier avec des élements de debug
 */
void Etalonnage::debugResult(Mat &output) {
    Point boueeRouge[] = {
            arig_utils::tablePtToImagePt(Point(670, 100)),
            arig_utils::tablePtToImagePt(Point(1100, 800)),
            arig_utils::tablePtToImagePt(Point(1730, 1200)),
            arig_utils::tablePtToImagePt(Point(2044, 400)),
            arig_utils::tablePtToImagePt(Point(2300, -67)),
            arig_utils::tablePtToImagePt(Point(1000, -67)),
    };

    Point boueeVerte[] = {
            arig_utils::tablePtToImagePt(Point(956, 400)),
            arig_utils::tablePtToImagePt(Point(1270, 1200)),
            arig_utils::tablePtToImagePt(Point(1900, 800)),
            arig_utils::tablePtToImagePt(Point(2330, 100)),
            arig_utils::tablePtToImagePt(Point(2000, -67)),
            arig_utils::tablePtToImagePt(Point(700, -67)),
    };

    for (auto &bouee : boueeRouge) {
        circle(output, bouee, 20, arig_utils::RED, 1);
    }
    for (auto &bouee : boueeVerte) {
        circle(output, bouee, 20, arig_utils::GREEN, 1);
    }

    circle(output, Point(750, 1000), 250, arig_utils::BLACK, 1);

    auto probe = arig_utils::getProbe(
            arig_utils::tablePtToImagePt(Point(1500, 20)),
            config->probeSize
    );
    rectangle(output, probe, arig_utils::BLUE, 1);
}

/**
 * Cherche le marker 42, retourne le centre du bord bas
 */
bool Etalonnage::detectMarker(const Mat &source, Mat &output, Point &pt) {
    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    aruco::detectMarkers(source, dictionary, markerCorners, markerIds);
    aruco::drawDetectedMarkers(output, markerCorners, markerIds);

    vector<Point2f> marker42;
    for (auto i = 0; i < markerIds.size(); i++) {
        if (markerIds.at(i) == 42) {
            marker42 = markerCorners.at(i);
            break;
        }
    }

    if (marker42.empty()) {
        spdlog::error("Cannot find the marker 42");
        return false;

    } else {
        pt.x = (marker42.at(0).x + marker42.at(1).x) / 2.0;
        pt.y = (marker42.at(0).y + marker42.at(1).y) / 2.0;
        return true;
    }
}

/**
 * Récupère les couleurs de référence rouge et vert
 */
bool Etalonnage::calibCouleurs(const Mat &source, Mat &output, const Point &markerCenter) {
    Point bouee9 = markerCenter + Point(-85, 0);
    Point bouee8 = markerCenter + Point(85, 0);

    Rect probeRed = arig_utils::getProbe(bouee9, config->probeSize);
    rectangle(output, probeRed.tl(), probeRed.br(), arig_utils::WHITE);

    Rect probeGreen = arig_utils::getProbe(bouee8, config->probeSize);
    rectangle(output, probeGreen.tl(), probeGreen.br(), arig_utils::WHITE);

    config->red = arig_utils::ScalarBGR2HSV(arig_utils::getAverageColor(source, probeRed));
    config->green = arig_utils::ScalarBGR2HSV(arig_utils::getAverageColor(source, probeGreen));

    return true;
}

/**
 * Récupère les couleurs de référence rouge et vert pour les ecueils
 */
bool Etalonnage::calibCouleursEcueil(const Mat &source, Mat &output) {
    auto probeGreenAdverse = arig_utils::getProbe(Detection::getEcueilPoint(config->team, true, 0), config->probeSize);
    auto probeRedAdverse = arig_utils::getProbe(Detection::getEcueilPoint(config->team, true, 4), config->probeSize);
    auto probeGreenEquipe = arig_utils::getProbe(Detection::getEcueilPoint(config->team, false, 4), config->probeSize);
    auto probeRedEquipe = arig_utils::getProbe(Detection::getEcueilPoint(config->team, false, 0), config->probeSize);

    rectangle(output, probeGreenAdverse.tl(), probeGreenAdverse.br(), arig_utils::WHITE);
    rectangle(output, probeRedAdverse.tl(), probeRedAdverse.br(), arig_utils::WHITE);
    rectangle(output, probeGreenEquipe.tl(), probeGreenEquipe.br(), arig_utils::WHITE);
    rectangle(output, probeRedEquipe.tl(), probeRedEquipe.br(), arig_utils::WHITE);

    Scalar greenAdverse = arig_utils::getAverageColor(source, probeGreenAdverse);
    Scalar redAdverse = arig_utils::getAverageColor(source, probeRedAdverse);
    Scalar greenEquipe = arig_utils::getAverageColor(source, probeGreenEquipe);
    Scalar redEquipe = arig_utils::getAverageColor(source, probeRedEquipe);

    config->redEcueil = arig_utils::ScalarBGR2HSV(arig_utils::meanBGR(redAdverse, redEquipe));
    config->greenEcueil = arig_utils::ScalarBGR2HSV(arig_utils::meanBGR(greenAdverse, greenEquipe));

    return true;
}

/**
 * Fait une detection de couleur pour trouver les balises extrêmes
 */
bool Etalonnage::detectBouees(const Mat &imageHsv, Mat &output,
                              const vector<Scalar> &colorRange, const vector<Point> &zone, bool sideIsMinX,
                              Point &boueeTop, Point &boueeSide) {
    Mat imageThreshold;
    arig_utils::hsvInRange(imageHsv, colorRange, imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    vector<vector<Point>> contours;
    findContours(imageThreshold, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // boueeTop : bouee de min Y
    // boueeSide : bouee de min/max X (selon sideIsMinX)
    vector<Point> contourBoueeTop, contourBoueeSide;
    double minY = config->cameraResolution.width;
    double minX = config->cameraResolution.height;
    double maxX = 0;

    for (auto i = 0; i < contours.size(); i++) {
        Moments moment = moments(contours.at(i));
        double area = moment.m00;

        if (area < 800 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        // filtrage dans le polygone de recherche
        if (pointPolygonTest(zone, Point(x, y), false) < 0) {
            continue;
        }

        if (sideIsMinX && x < minX) {
            minX = x;
            contourBoueeSide = contours.at(i);
        }
        if (!sideIsMinX && x > maxX) {
            maxX = x;
            contourBoueeSide = contours.at(i);
        }
        if (y < minY) {
            minY = y;
            contourBoueeTop = contours.at(i);
        }

        drawContours(output, contours, i, arig_utils::BLACK, 2);
        putText(output, to_string((int) area), Point(x, y), 0, 0.5, arig_utils::WHITE);
    }

    if (contourBoueeTop.empty() || contourBoueeSide.empty()) {
        spdlog::error("Cannot find color clusters");
        return false;
    }

    // on cherche la position de la base de chaque bouee
    // x = x milieu des points de la bordure basse du contours (max y)
    // y = y milieu des points de la bordure gauche ou droite (min x ou max x)
    //      selon si la bouee est à droite ou gauche de l'image, respectivement

    boueeTop.x = arig_utils::averageX(arig_utils::pointsOfMaxY(contourBoueeTop));
    if (contourBoueeTop.at(0).x > config->cameraResolution.width / 2.0) {
        boueeTop.y = arig_utils::averageY(arig_utils::pointsOfMinX(contourBoueeTop));
    } else {
        boueeTop.y = arig_utils::averageY(arig_utils::pointsOfMaxX(contourBoueeTop));
    }

    boueeSide.x = arig_utils::averageX(arig_utils::pointsOfMaxY(contourBoueeSide));
    if (contourBoueeSide.at(0).x > config->cameraResolution.width / 2.0) {
        boueeSide.y = arig_utils::averageY(arig_utils::pointsOfMinX(contourBoueeSide));
    } else {
        boueeSide.y = arig_utils::averageY(arig_utils::pointsOfMinX(contourBoueeSide));
    }

    return true;
}
