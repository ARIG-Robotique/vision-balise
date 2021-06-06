#include "Etalonnage.h"
#include "utils.h"

Etalonnage::Etalonnage(Config *config) {
    this->config = config;
}

JsonResult Etalonnage::run(const Mat &source) {
    auto start = arig_utils::startTiming();
    spdlog::info("ETALONNAGE {}", ++index);

    Mat output = source.clone();
    JsonResult r;

    // DETECTION DU MARKER
    Point markerCenter;
    if (!detectMarker(source, output, markerCenter)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de trouver le marqueur";
        return r;
    }

    spdlog::debug("Marker 42 found : {}", markerCenter);

    config->team = markerCenter.x > config->cameraResolution.width / 2.0 ? TEAM_BLEU : TEAM_JAUNE;
    spdlog::debug("Équipe {}", config->team);


    // CALIBRATION DES COULEURS
    if (!calibCouleurs(source, output, markerCenter)) {
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
    if (!detectBouees(imageHsv, output, redRange, detectionZone, false, bouee8, bouee12) ||
        !detectBouees(imageHsv, output, greenRange, detectionZone, true, bouee9, bouee5)) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de détecter les bouées";
        return r;
    }

    spdlog::debug("Bouee 8 : {}", bouee8);
    spdlog::debug("Bouee 12 : {}", bouee12);
    spdlog::debug("Bouee 9 : {}", bouee9);
    spdlog::debug("Bouee 5 : {}", bouee5);

    polylines(output, detectionZone, true, arig_utils::BLUE, 1);
    circle(output, bouee8, 20, arig_utils::RED, 1);
    circle(output, bouee12, 20, arig_utils::RED, 1);
    circle(output, bouee9, 20, arig_utils::GREEN, 1);
    circle(output, bouee5, 20, arig_utils::GREEN, 1);


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

    imwrite(config->outputPrefix + "etallonage-" + to_string(index) + ".jpg", output);

    Mat result = debugResult(source);

    r.status = RESPONSE_OK;
    r.data = arig_utils::matToBase64(result);

    config->etalonnageDone = true;

    spdlog::debug("Etalonnage en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

/**
 * Applique la correction de perspective et écrit un fichier avec des élements de debug
 */
Mat Etalonnage::debugResult(const Mat &source) {
    Mat result;
    warpPerspective(source, result, config->perspectiveMap, config->perspectiveSize);

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
        circle(result, bouee, 20, arig_utils::RED, 1);
    }
    for (auto &bouee : boueeVerte) {
        circle(result, bouee, 20, arig_utils::GREEN, 1);
    }

    circle(result, Point(750, 1000), 250, arig_utils::BLACK, 1);

    auto probe = arig_utils::getProbe(
            arig_utils::tablePtToImagePt(Point(1500, 20)),
            config->probeSize
    );
    rectangle(result, probe, arig_utils::BLUE, 1);

    imwrite(config->outputPrefix + "etallonage-result-" + to_string(index) + ".jpg", result);

    return result;
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
