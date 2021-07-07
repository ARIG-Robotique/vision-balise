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

    Mat work = undistorted.clone();

    // detection marker
    vector<int> markerIds;
    vector<vector<Point2f>> markerCorners;
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    aruco::detectMarkers(undistorted, dictionary, markerCorners, markerIds);
    aruco::drawDetectedMarkers(work, markerCorners, markerIds);

    vector<Point2f> marker42;
    for (unsigned long i = 0; i < markerIds.size(); i++) {
        if (markerIds.at(i) == 42) {
            marker42 = markerCorners.at(i);
            break;
        }
    }

    if (marker42.empty()) {
        spdlog::error("Cannot find the marker");
        return;
    }

    // x=1500 y=1200
    Point2f markerCenter(
            (marker42.at(0).x + marker42.at(1).x) / 2.0,
            (marker42.at(0).y + marker42.at(1).y) / 2.0
    );

    line(work,
         Point(config->cameraResolution.width / 2, 0),
         Point(config->cameraResolution.width / 2, config->cameraResolution.height),
         Scalar(0, 0, 255)
    );

    int offsetXMarker =  markerCenter.x - config->cameraResolution.width / 2;
    vector<vector<Point>> detectionZone = {
            {
                    Point(offsetXMarker + config->cameraResolution.width / 2 - 150, 100),
                    Point(offsetXMarker + config->cameraResolution.width / 2 + 150, 100),
                    Point(offsetXMarker + config->cameraResolution.width - 50, config->cameraResolution.height - 250),
                    Point(offsetXMarker + 50, config->cameraResolution.height - 250),
            }
    };

    drawContours(work, detectionZone, 0, Scalar(0, 255, 0));

    string team = markerCenter.x > config->cameraResolution.width / 2.0 ? TEAM_BLEU : TEAM_JAUNE;
    spdlog::debug("Coté {}", team == TEAM_BLEU ? "BLEU" : "JAUNE");

    show(work, debugAll);

    // IMAGE LAB
//    Mat lab;
//    cvtColor(undistorted, lab, COLOR_BGR2Lab);
//
//    Mat lab_planes[3];
//    split(lab, lab_planes);
//
//    Mat labA = lab_planes[1];
//    auto clahe = createCLAHE(4, Size(8, 8));
//    clahe->apply(lab_planes[1], labA);
//
//    Mat workLab = labA.clone();
//
//    // CALIBRATION COULEUR
//    Point2f bouee9Temp = markerCenter + Point2f(-85, 0);
//    Point2f bouee8Temp = markerCenter + Point2f(85, 0);
//
//    Rect probeRed = arig_utils::getProbe(bouee9Temp, 15);
//    rectangle(workLab, probeRed.tl(), probeRed.br(), Scalar(255, 255, 255));
//
//    Rect probeGreen = arig_utils::getProbe(bouee8Temp, 15);
//    rectangle(workLab, probeGreen.tl(), probeGreen.br(), Scalar(255, 255, 255));
//
//    Scalar redLab = arig_utils::getAverageColor(labA, probeRed);
//    Scalar greenLab = arig_utils::getAverageColor(labA, probeGreen);
//
//    spdlog::debug("Rouge {}", redLab[0]);
//    spdlog::debug("Vert {}", greenLab[0]);
//
//    show(workLab, true);
//
//    Mat thresholdLab;
//
//    inRange(labA, max(0.0, redLab[0] - 15), min(255.0, redLab[0] + 15), thresholdLab);
//    dilate(thresholdLab, thresholdLab, Mat(), Point(-1, -1), 2);
//    erode(thresholdLab, thresholdLab, Mat(), Point(-1, -1), 2);
//
//    show(thresholdLab, true);
//
//    inRange(labA, max(0.0, greenLab[0] - 15), min(255.0, greenLab[0] + 15), thresholdLab);
//    dilate(thresholdLab, thresholdLab, Mat(), Point(-1, -1), 2);
//    erode(thresholdLab, thresholdLab, Mat(), Point(-1, -1), 2);
//
//    show(thresholdLab, true);

    // CALIBRATION COULEUR
    Point2f bouee9Temp = markerCenter + Point2f(-85, 0);
    Point2f bouee8Temp = markerCenter + Point2f(85, 0);

    Rect probeRed = arig_utils::getProbe(bouee9Temp, 15);
    rectangle(work, probeRed.tl(), probeRed.br(), Scalar(255, 255, 255));

    Rect probeGreen = arig_utils::getProbe(bouee8Temp, 15);
    rectangle(work, probeGreen.tl(), probeGreen.br(), Scalar(255, 255, 255));

    Scalar red = arig_utils::ScalarBGR2HSV(arig_utils::getAverageColor(undistorted, probeRed));
    Scalar green = arig_utils::ScalarBGR2HSV(arig_utils::getAverageColor(undistorted, probeGreen));

    spdlog::debug("Rouge {}", red);
    spdlog::debug("Vert {}", green);

    show(work, debugAll);


    // DETECTION BLOBS
    work = undistorted.clone();

    Mat imageHsv, imageThreshold;
    cvtColor(undistorted, imageHsv, COLOR_BGR2HSV);

    // H +- 10
    // S +- 75
    // V +- 100
    auto hMin = red[0] - 10;
    if (hMin < 0) {
        hMin += 180;
    }
    auto hMax = red[0] + 10;
    if (hMax > 180) {
        hMax -= 180;
    }

    vector<Scalar> redRange = {
            Scalar(hMin, max(red[1] - 75, 0.0), max(red[2] - 100, 0.0)),
            Scalar(hMax, min(red[1] + 75, 255.0), min(red[2] + 100, 255.0)),
    };

    // H +- 10
    // S +- 75
    // V +- 50
    vector<Scalar> greenRange = {
            Scalar(green[0] - 10, max(green[1] - 75, 0.0), max(green[2] - 100, 0.0)),
            Scalar(green[0] + 10, min(green[1] + 75, 255.0), min(green[2] + 100, 255.0))
    };

    // RED
    arig_utils::hsvInRange(imageHsv, redRange, imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    show(imageThreshold, debugAll);

    vector<vector<Point>> contoursRed;
    findContours(imageThreshold, contoursRed, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    vector<Point> bouee8; // blob rouge de min Y
    vector<Point> bouee12; // blob rouge de max X
    double minY = config->cameraResolution.width;
    double maxX = 0;

    for (auto i = 0; i < contoursRed.size(); i++) {
        Moments moment = moments(contoursRed.at(i));
        double area = moment.m00;

        if (area < 800 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        if (pointPolygonTest(detectionZone.at(0), Point(x, y), false) < 0) {
            continue;
        }

        if (x > maxX) {
            maxX = x;
            bouee12 = contoursRed.at(i);
        }
        if (y < minY) {
            minY = y;
            bouee8 = contoursRed.at(i);
        }

        drawContours(work, contoursRed, i, Scalar(0, 0, 0), 2);
        putText(work, to_string((int) area), Point(x, y), 0, 0.5, Scalar(255, 255, 255));
    }

    show(work, debugAll);

    vector<Point> bouee8PtX = arig_utils::pointsOfMaxX(bouee8);
    vector<Point> bouee8PtY = arig_utils::pointsOfMaxY(bouee8);
    Point2f bouee8Pt(arig_utils::averageX(bouee8PtY), arig_utils::averageY(bouee8PtX));

    vector<Point> bouee12PtX = arig_utils::pointsOfMinX(bouee12);
    vector<Point> bouee12PtY = arig_utils::pointsOfMaxY(bouee12);
    Point2f bouee12Pt(arig_utils::averageX(bouee12PtY), arig_utils::averageY(bouee12PtX));

    spdlog::debug("bouee8 {}", bouee8Pt);
    spdlog::debug("bouee12 {}", bouee12Pt);

    circle(work, bouee8Pt, 20, Scalar(0, 0, 255), 2);
    circle(work, bouee12Pt, 20, Scalar(0, 0, 255), 2);

    show(work, debugAll);


    // GREEN
    inRange(imageHsv, greenRange.at(0), greenRange.at(1), imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    show(imageThreshold, debugAll);

    vector<vector<Point>> contoursGreen;
    findContours(imageThreshold, contoursGreen, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    vector<Point> bouee9; // blob vert de min Y
    vector<Point> bouee5; // blob vert de min X
    minY = config->cameraResolution.width;
    double minX = config->cameraResolution.height;

    for (auto i = 0; i < contoursGreen.size(); i++) {
        Moments moment = moments(contoursGreen.at(i));
        double area = moment.m00;

        if (area < 800 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        if (pointPolygonTest(detectionZone.at(0), Point(x, y), false) < 0) {
            continue;
        }

        if (x < minX) {
            minX = x;
            bouee5 = contoursGreen.at(i);
        }
        if (y < minY) {
            minY = y;
            bouee9 = contoursGreen.at(i);
        }

        drawContours(work, contoursGreen, i, Scalar(0, 0, 0), 2);
        putText(work, to_string((int) area), Point(x, y), 0, 0.5, Scalar(255, 255, 255));
    }

    show(work, debugAll);

    vector<Point> bouee9PtX = arig_utils::pointsOfMinX(bouee9);
    vector<Point> bouee9PtY = arig_utils::pointsOfMaxY(bouee9);
    Point2f bouee9Pt(arig_utils::averageX(bouee9PtY), arig_utils::averageY(bouee9PtX));

    vector<Point> bouee5PtX = arig_utils::pointsOfMaxX(bouee5);
    vector<Point> bouee5PtY = arig_utils::pointsOfMaxY(bouee5);
    Point2f bouee5Pt(arig_utils::averageX(bouee5PtY), arig_utils::averageY(bouee5PtX));

    spdlog::debug("bouee9 {}", bouee9Pt);
    spdlog::debug("bouee5 {}", bouee5Pt);

    circle(work, bouee9Pt, 20, Scalar(0, 255, 0), 2);
    circle(work, bouee5Pt, 20, Scalar(0, 255, 0), 2);

    show(work, debugAll);


    // PERSPECTIVE
    Point2f ptsImages[] = {bouee9Pt, bouee8Pt, bouee5Pt, bouee12Pt};
    Point2f ptsProj[] = {
            Point2f((3000 - 1270) / 2.0, (2000 - 1200) / 2.0),
            Point2f((3000 - 1730) / 2.0, (2000 - 1200) / 2.0),
            Point2f((3000 - 2330) / 2.0, (2000 - 100) / 2.0),
            Point2f((3000 - 670) / 2.0, (2000 - 100) / 2.0)
    };

    Mat proj = getPerspectiveTransform(ptsImages, ptsProj);

    Mat projected;
    warpPerspective(undistorted, projected, proj, Size(1500, 1100));

    show(projected, debugAll);

    // CALIBRATION COULEUR ECUEIL
    int y = -67;
    int offsetY = 5;
    int xRedAdverse = 2000;
    int xGreenAdverse = 2000 + 4 * 75;
    int xRedEquipe = 1000;
    int xGreenEquipe = 1000 - 4 * 75;
    int offsetXRedAdvserse = -30;
    int offsetXGreenAdvserse = -30 - 20;
    int offsetXRedEquipe = 20;
    int offsetXGreenEquipe = 20 + 20;
    if (team == TEAM_JAUNE) {
        xRedAdverse = 3000 - xRedAdverse;
        xGreenAdverse = 3000 - xGreenAdverse;
        xRedEquipe = 3000 - xRedEquipe;
        xGreenEquipe = 3000 - xGreenEquipe;
        offsetXRedAdvserse *= -1;
        offsetXGreenAdvserse *= -1;
        offsetXRedEquipe *= -1;
        offsetXGreenEquipe *= -1;
    }

    auto probe1 = arig_utils::getProbe(arig_utils::tablePtToImagePt(Point(xRedAdverse, y)) + Point(offsetXRedAdvserse, offsetY), 15);
    auto probe2 = arig_utils::getProbe(arig_utils::tablePtToImagePt(Point(xGreenAdverse, y)) + Point(offsetXGreenAdvserse, offsetY), 15);
    auto probe3 = arig_utils::getProbe(arig_utils::tablePtToImagePt(Point(xRedEquipe, y)) + Point(offsetXRedEquipe, offsetY), 15);
    auto probe4 = arig_utils::getProbe(arig_utils::tablePtToImagePt(Point(xGreenEquipe, y)) + Point(offsetXGreenEquipe, offsetY), 15);

    Scalar greenAdverse = arig_utils::getAverageColor(projected, probe1);
    Scalar redAdverse = arig_utils::getAverageColor(projected, probe2);
    Scalar redEquipe = arig_utils::getAverageColor(projected, probe3);
    Scalar greenEquipe = arig_utils::getAverageColor(projected, probe4);

    Scalar redEcueil = arig_utils::ScalarBGR2HSV(arig_utils::meanBGR(redAdverse, redEquipe));
    Scalar greenEcueil = arig_utils::ScalarBGR2HSV(arig_utils::meanBGR(greenAdverse, greenEquipe));

    spdlog::debug("Rouge ecueil {}", redEcueil);
    spdlog::debug("Vert ecueil {}", greenEcueil);

    work = projected.clone();

    rectangle(work, probe1, Scalar(255, 255, 255), 1);
    rectangle(work, probe2, Scalar(255, 255, 255), 1);
    rectangle(work, probe3, Scalar(255, 255, 255), 1);
    rectangle(work, probe4, Scalar(255, 255, 255), 1);

    show(work, debugAll);

    work = projected.clone();

    circle(work, Point2f((3000 - 670) / 2.0, (2000 - 100) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 1100) / 2.0, (2000 - 800) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 1730) / 2.0, (2000 - 1200) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 2044) / 2.0, (2000 - 400) / 2.0), 20, Scalar(0, 0, 255), 1);

    circle(work, Point2f((3000 - 2300) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 2150) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 2075) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 1000) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 0, 255), 1);
    circle(work, Point2f((3000 - 775) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 0, 255), 1);

    circle(work, Point2f((3000 - 956) / 2.0, (2000 - 400) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 1270) / 2.0, (2000 - 1200) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 1900) / 2.0, (2000 - 800) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 2330) / 2.0, (2000 - 100) / 2.0), 20, Scalar(0, 255, 0), 1);

    circle(work, Point2f((3000 - 2225) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 2000) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 925) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 850) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 255, 0), 1);
    circle(work, Point2f((3000 - 700) / 2.0, (2000 + 67) / 2.0), 20, Scalar(0, 255, 0), 1);

    circle(work, Point(750, 1000), 250, Scalar(0, 0, 0), 1);

    show(work, debugAll);


    // LECTURE GIROUTETTE
    Rect probeGir = arig_utils::getProbe(Point(1500 / 2.0, (2000 - 20) / 2.0), 20);
    auto colorGir = arig_utils::getAverageColor(projected, probeGir);
    auto valueGir = arig_utils::ScalarBGR2HSV(colorGir)[2];

    string resGir;
    if (valueGir > 150) {
        resGir = DIR_DOWN;
    } else if (valueGir < 100) {
        resGir = DIR_UP;
    } else {
        resGir = DIR_UNKNOWN;
    }

    rectangle(work, probeGir, Scalar(255, 0, 0), 1);
    putText(work, resGir, probeGir.tl(), 0, 0.5, Scalar(255, 255, 255));
    spdlog::debug("Girouette {}", resGir);

    show(work, debugAll);


    // LECTURE ECUEIL
    // equipe
    vector<Rect> probes;
    for (auto i = 0; i < 5; i++) {
        auto x = team == TEAM_BLEU ? (1000 - i * 75) : (2000 + i * 75);
        auto y = -67;
        auto offsetX = (team == TEAM_BLEU ? 1 : -1) * (20 + 20 * i / 4.0); // offset de 20 à 40 pixels sur x
        auto offsetY = 5;
        probes.emplace_back(
                arig_utils::getProbe(Point2f((3000 - x) / 2.0 + offsetX, (2000 - y) / 2.0 + offsetY), 15));
    }

    vector<string> ecueil;
    for (auto &probe : probes) {
        auto color = arig_utils::getAverageColor(projected, probe);
        auto hue = arig_utils::ScalarBGR2HSV(color)[0];

        auto dRed = abs(hue - redEcueil[0]);
        dRed = min(dRed, 180 - dRed);
        auto dGreen = abs(hue - greenEcueil[0]);
        dGreen = min(dGreen, 180 - dGreen);

        string res;
        if (dRed < config->colorThreshold) {
            res = COLOR_RED;
        } else if (dGreen < config->colorThreshold) {
            res = COLOR_GREEN;
        } else {
            res = COLOR_UNKNOWN;
        }
        ecueil.emplace_back(res);

        rectangle(work, probe, Scalar(255, 255, 255), 1);
        putText(work, res, probe.tl(), 0, 0.5, Scalar(255, 255, 255));
    }

    // adverse
    probes.clear();
    for (auto i = 0; i < 5; i++) {
        auto x = team == TEAM_BLEU ? (2000 + i * 75) : (1000 - i * 75);
        auto y = -67;
        auto offsetX = (team == TEAM_BLEU ? -1 : 1) * (30 + 20 * i / 4.0); // offset de 30 à 50 pixels sur x
        auto offsetY = 5;
        probes.emplace_back(
                arig_utils::getProbe(Point2f((3000 - x) / 2.0 + offsetX, (2000 - y) / 2.0 + offsetY), 15));
    }

    for (auto &probe : probes) {
        auto color = arig_utils::getAverageColor(projected, probe);
        auto hue = arig_utils::ScalarBGR2HSV(color)[0];

        auto dRed = abs(hue - redEcueil[0]);
        dRed = min(dRed, 180 - dRed);
        auto dGreen = abs(hue - greenEcueil[0]);
        dGreen = min(dGreen, 180 - dGreen);

        string res;
        if (dRed < config->colorThreshold) {
            res = COLOR_RED;
        } else if (dGreen < config->colorThreshold) {
            res = COLOR_GREEN;
        } else {
            res = COLOR_UNKNOWN;
        }

        rectangle(work, probe, Scalar(255, 255, 255), 1);
        putText(work, res, probe.tl(), 0, 0.5, Scalar(255, 255, 255));
    }

    spdlog::debug("Ecueil {}", arig_utils::strings2json(ecueil).dump());

    show(work, debugAll);


    // PRESENCE BOUEE
    vector<pair<Scalar, Point2f>> bouees = {
            {red,   Point2f((3000 - 670) / 2.0, (2000 - 100) / 2.0)},
            {red,   Point2f((3000 - 1100) / 2.0, (2000 - 800) / 2.0)},
            {red,   Point2f((3000 - 1730) / 2.0, (2000 - 1200) / 2.0)},
            {red,   Point2f((3000 - 2044) / 2.0, (2000 - 400) / 2.0)},
            {green, Point2f((3000 - 956) / 2.0, (2000 - 400) / 2.0)},
            {green, Point2f((3000 - 1270) / 2.0, (2000 - 1200) / 2.0)},
            {green, Point2f((3000 - 1900) / 2.0, (2000 - 800) / 2.0)},
            {green, Point2f((3000 - 2330) / 2.0, (2000 - 100) / 2.0)}
    };

    for (auto &bouee : bouees) {
        auto probe = arig_utils::getProbe(bouee.second, 15);
        auto color = arig_utils::getAverageColor(projected, probe);
        auto hue = arig_utils::ScalarBGR2HSV(color)[0];
        auto d = abs(hue - bouee.first[0]);
        d = min(d, 180 - d);

        string res;
        if (d < config->colorThreshold) {
            res = "PRESENT";
        } else {
            res = "ABSENT";
        }

        rectangle(work, probe, Scalar(255, 255, 255), 1);
        putText(work, res, probe.tl(), 0, 0.5, Scalar(255, 255, 255));
    }

    show(work, debugAll);


    // ZONE DE DESTRUCTION !!!
    cvtColor(projected, imageHsv, COLOR_BGR2HSV);

    // rouge
    arig_utils::hsvInRange(imageHsv, redRange, imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    contoursRed.clear();
    findContours(imageThreshold, contoursRed, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    for (auto i = 0; i < contoursRed.size(); i++) {
        Moments moment = moments(contoursRed.at(i));
        double area = moment.m00;

        if (area < 800 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        double dst = pow(750 - x, 2) + pow(1000 - y, 2);
        if (dst > 250 * 250 || y > 1000) {
            continue;
        }

        auto ptX = x > 750 ? arig_utils::pointsOfMinX(contoursRed.at(i)) : arig_utils::pointsOfMaxX(contoursRed.at(i));
        auto ptY = arig_utils::pointsOfMaxY(contoursRed.at(i));

        auto pt = Point2f(arig_utils::averageX(ptY), arig_utils::averageY(ptX));

        drawContours(work, contoursRed, i, Scalar(0, 0, 0), 2);
        circle(work, pt, 20, arig_utils::RED, 1);
        String txt = to_string((int) (3000 - pt.x * 2)) + "x" + to_string((int) (pt.y * 2)) + " : RED";
        putText(work, txt, pt, 0, 0.5, Scalar(255, 255, 255));
    }

    // vert
    inRange(imageHsv, greenRange.at(0), greenRange.at(1), imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    contoursGreen.clear();
    findContours(imageThreshold, contoursGreen, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    for (auto i = 0; i < contoursGreen.size(); i++) {
        Moments moment = moments(contoursGreen.at(i));
        double area = moment.m00;

        if (area < 100 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        double dst = pow(750 - x, 2) + pow(1000 - y, 2);
        if (dst > 250 * 250 || y > 1000) {
            continue;
        }

        auto ptX =
                x > 750 ? arig_utils::pointsOfMinX(contoursGreen.at(i)) : arig_utils::pointsOfMaxX(contoursGreen.at(i));
        auto ptY = arig_utils::pointsOfMaxY(contoursGreen.at(i));

        auto pt = Point2f(arig_utils::averageX(ptY), arig_utils::averageY(ptX));

        drawContours(work, contoursGreen, i, Scalar(0, 0, 0), 2);
        circle(work, pt, 20, arig_utils::GREEN, 1);
        String txt = to_string((int) (3000 - pt.x * 2)) + "x" + to_string((int) (pt.y * 2)) + " : GREEN";
        putText(work, txt, pt, 0, 0.5, Scalar(255, 255, 255));
    }

    show(work, true);
}

#endif //VISION_BALISE_TEST_H
