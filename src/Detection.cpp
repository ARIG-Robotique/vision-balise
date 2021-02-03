#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;

    for (auto i = 0; i < 8; i++) {
        boueesBuffer.emplace_back(vector<int>(config->detectionBuffer, 0));
    }
    girouetteBuffer = vector<int>(config->detectionBuffer, 0);
}

json Detection::run(const Mat &source, int index) {
    auto start = arig_utils::startTiming();
    spdlog::info("DETECTION {}", index);

    json r;

    Mat projected, imageHsv, output;
    warpPerspective(source, projected, config->perspectiveMap, config->perspectiveSize);
    cvtColor(projected, imageHsv, COLOR_BGR2HSV);

    if (config->debug || index % 10 == 0) {
        output = projected.clone();
    }

    // LECTURE GIROUETTE
    vector<string> girouette;
    if (lectureGirouette(imageHsv, output, girouette)) {
        r["girouette"] = girouette.at(0);
    }

    // LECTURE ECUEILS
    vector<string> ecueilEquipe, ecueilAdverse;
    if (lectureEcueil(imageHsv, output, false, ecueilEquipe) &&
        lectureEcueil(imageHsv, output, true, ecueilAdverse)) {
        // TODO nettoyage des données erronées à cause de la perspective
        r["ecueilEquipe"] = arig_utils::strings2json(ecueilEquipe);
        r["ecueilAdverse"] = arig_utils::strings2json(ecueilAdverse);
    }

    // CONTROLE BOUEES
    vector<string> bouees;
    if (controleBouees(imageHsv, output, bouees)) {
        r["bouees"] = arig_utils::strings2json(bouees);
    }

    // DETECTION BOUEES
    vector<Scalar> redRange = config->getRedRange();
    vector<Scalar> greenRange = config->getGreenRange();

    vector<pair<string, Point>> hautFond;
    if (detectionBouees(imageHsv, output, redRange, hautFond) &&
        detectionBouees(imageHsv, output, greenRange, hautFond)) {
        json rHautFond;

        for (auto &tmp : hautFond) {
            json tmp2;
            tmp2["col"] = tmp.first;
            tmp2["pos"] = json({tmp.second.x, tmp.second.y});
            rHautFond.emplace_back(tmp2);
        }

        r["hautFond"] = rHautFond;
    }

    bufferIndex++;
    if (bufferIndex == config->detectionBuffer) {
        bufferIndex = 0;
    }

    if (!output.empty()) {
        imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", output);
    }

    spdlog::debug(r.dump(2));
    spdlog::debug("Détection en {}ms", arig_utils::ellapsedTime(start));

    return r;
}

/**
 * Lit la direction de la girouette
 */
bool Detection::lectureGirouette(const Mat &imageHsv, Mat &output, vector<string> &girouette) {
    auto probe = arig_utils::getProbe(
            arig_utils::tablePtToImagePt(Point(1500, 20)),
            config->probeSize
    );
    auto color = arig_utils::getAverageColor(imageHsv, probe);

    if (color[2] > 150) {
        girouetteBuffer[bufferIndex] = 1;
    } else if (color[2] < 100) {
        girouetteBuffer[bufferIndex] = -1;
    } else {
        girouetteBuffer[bufferIndex] = 0;
    }

    auto val = 0.0;
    for (auto i = 0; i < config->detectionBuffer; i++) {
        val += girouetteBuffer[i];
    }

    string res;
    if (val >= config->detectionValidLimit) {
        res = DIR_DOWN;
    } else if (val <= -config->detectionValidLimit) {
        res = DIR_UP;
    } else {
        res = DIR_UNKNOWN;
    }

    if (!output.empty()) {
        rectangle(output, probe, arig_utils::BLUE, 1);
        putText(output, res, probe.tl(), 0, 0.5, arig_utils::WHITE);
    }

    return true;
}

/**
 * Lit les couleurs d'un ecueil
 */
bool Detection::lectureEcueil(const Mat &imageHsv, Mat &output, bool adverse, vector<string> &ecueil) {
    vector<Rect> probes;
    for (auto i = 0; i < 5; i++) {
        // x et y dans le coordonnées de la table
        int x = adverse ? (2000 + i * 75) : (1000 - i * 75);
        int y = -67;
        if (config->team == TEAM_JAUNE) {
            x = 3000 - x;
        }

        // offset x et y en pixels sur l'image
        int offsetX = adverse ? -(30 + 20 * i / 4.0) : (20 + 20 * i / 4.0);
        int offsetY = 5;
        if (config->team == TEAM_JAUNE) {
            offsetX *= -1;
        }

        auto probe = arig_utils::tablePtToImagePt(Point(x, y)) + Point(offsetX, offsetY);
        probes.emplace_back(arig_utils::getProbe(probe, 15));
    }

    for (auto &probe : probes) {
        auto color = arig_utils::getAverageColor(imageHsv, probe);

        auto dRed = abs(color[0] - config->red[0]);
        dRed = min(dRed, 180 - dRed);
        auto dGreen = abs(color[0] - config->green[0]);
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

        if (!output.empty()) {
            rectangle(output, probe, arig_utils::WHITE, 1);
            putText(output, res, probe.tl(), 0, 0.5, arig_utils::WHITE);
        }
    }

    return true;
}

/**
 * Controle la présence des bouees centrales
 */
bool Detection::controleBouees(const Mat &imageHsv, Mat &output, vector<string> &bouees) {
    // 5 to 12
    vector<pair<Scalar, Point2f>> positions = {
            {config->green, Point(2330, 100)},
            {config->red,   Point(2044, 400)},
            {config->green, Point(1900, 800)},
            {config->red,   Point(1730, 1200)},
            {config->green, Point(1270, 1200)},
            {config->red,   Point(1100, 800)},
            {config->green, Point(956, 400)},
            {config->red,   Point(670, 100)},
    };

    for (auto i = 0; i < positions.size(); i++) {
        auto position = positions.at(i);
        auto probe = arig_utils::getProbe(arig_utils::tablePtToImagePt(position.second),
                                          config->probeSize);
        auto color = arig_utils::getAverageColor(imageHsv, probe);

        auto d = abs(color[0] - position.first[0]);
        d = min(d, 180 - d);

        if (d < config->colorThreshold) {
            boueesBuffer[i][bufferIndex] = 1;
        } else {
            boueesBuffer[i][bufferIndex] = 0;
        }

        auto val = 0.0;
        for (auto j = 0; j < config->detectionBuffer; j++) {
            val += boueesBuffer[i][j];
        }

        string res;
        if (val >= config->detectionValidLimit) {
            res = BOUE_PRESENT;
        } else {
            res = BOUE_ABSENT;
        }
        bouees.emplace_back(res);

        if (!output.empty()) {
            rectangle(output, probe, arig_utils::WHITE, 1);
            putText(output, res, probe.tl(), 0, 0.5, arig_utils::WHITE);
        }
    }

    return true;
}

/**
 * Recherche les bouees dans le haut fond
 */
bool Detection::detectionBouees(const Mat &imageHsv, Mat &output, const vector<Scalar> &colorRange,
                                vector<pair<string, Point>> &bouees) {
    Mat imageThreshold;
    arig_utils::hsvInRange(imageHsv, colorRange, imageThreshold);
    erode(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);
    dilate(imageThreshold, imageThreshold, Mat(), Point(-1, -1), 2);

    vector<vector<Point>> contours;
    findContours(imageThreshold, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    for (auto i = 0; i < contours.size(); i++) {
        auto contour = contours.at(i);
        Moments moment = moments(contour);
        double area = moment.m00;

        if (area < 800 || area > 3000) {
            continue;
        }

        double x = moment.m10 / area;
        double y = moment.m01 / area;

        // filtrage dans la zone de hauts fonds
        double dst = pow(750 - x, 2) + pow(1000 - y, 2);
        if (dst > 250 * 250 || y > 1000) {
            continue;
        }

        Point pt;
        pt.x = arig_utils::averageX(arig_utils::pointsOfMaxY(contour));
        if (x > config->perspectiveSize.width / 2.0) {
            pt.y = arig_utils::averageY(arig_utils::pointsOfMinX(contour));
        } else {
            pt.y = arig_utils::averageY(arig_utils::pointsOfMaxX(contour));
        }

        auto probe = arig_utils::getProbe(pt, config->probeSize);
        auto color = arig_utils::getAverageColor(imageHsv, probe);

        auto dRed = abs(color[0] - config->red[0]);
        dRed = min(dRed, 180 - dRed);
        auto dGreen = abs(color[0] - config->green[0]);
        dGreen = min(dGreen, 180 - dGreen);

        string res;
        if (dRed < config->colorThreshold) {
            res = COLOR_RED;
        } else if (dGreen < config->colorThreshold) {
            res = COLOR_GREEN;
        } else {
            res = COLOR_UNKNOWN;
        }

        const Point tablePt = arig_utils::imagePtToTablePt(pt);
        bouees.emplace_back(make_pair(res, tablePt));

        if (!output.empty()) {
            drawContours(output, contours, i, arig_utils::BLACK, 2);
            rectangle(output, probe, arig_utils::WHITE, 1);
            String txt = to_string(tablePt.x) + "x" + to_string(tablePt.y) + " : " + res;
            putText(output, txt, probe.tl(), 0, 0.5, arig_utils::WHITE);
        }
    }

    return true;
}
