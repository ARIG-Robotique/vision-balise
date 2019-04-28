#include "Etallonage.h"
#include "utils.h"

Etallonage::Etallonage(Config *config) {
    this->config = config;
}

bool Etallonage::run(const Mat &source, int index) {
    spdlog::info("Etalonnage {}", index);

    Mat output;

    // correction de l'objectif
    Mat undistorted;
    undistort(source, undistorted, config->cameraMatrix, config->distCoeffs);

    // recherche des marqueurs
    vector<int> markerIds;
    vector<Point2f> markerCorners;
    findMarkers(undistorted, markerCorners, markerIds);

    if (markerIds.size() < 4) {
        spdlog::warn("No markers found");
        return false;
    }

    // calcul de l'homography
    findMarkersHomography(markerCorners, markerIds, config->homography);

    vector<Point2f> transformedCorners;
    perspectiveTransform(markerCorners, transformedCorners, config->homography);

    spdlog::debug("MAKERS POSITIONS\n {}", transformedCorners);

    // correction de la perspective
    warpPerspective(undistorted, undistorted, config->homography,
                    Size(config->boardSize.width * config->boardRatio, config->boardSize.height * config->boardRatio));

    // calibration des couleurs
    readColors(undistorted);

    if (config->debug) {
        drawMarkerCorners(undistorted, transformedCorners, markerIds);
        drawColors(undistorted);
        imwrite(config->outputPrefix + "etallonage-" + to_string(index) + ".jpg", undistorted);
    }

    if (config->testMode) {
        imshow("Process", undistorted);
        waitKey(0);
    }

    return true;
}

/**
 * Lance un etallonage et sauvegarde dans un fichier
 * @param filename
 */
bool Etallonage::runAndSave(const String &filename) {
    VideoCapture video(config->cameraIndex);

    Mat source;
    video.read(source);

    if (!source.data) {
        spdlog::error("Could not open or find the image");
        return false;
    }

    if (run(source, 0)) {
        FileStorage fs(filename, FileStorage::WRITE);
        fs << "homography" << config->homography;
        fs << "blue" << config->blue;
        fs << "red" << config->red;
        fs << "green" << config->green;
        fs.release();

        return true;
    }

    return false;
}

/**
 * Trouve les marqueurs ARUCO sur l'image
 * @param image
 * @param corners
 * @param markerIds
 */
void Etallonage::findMarkers(const Mat &image, vector<Point2f> &corners, vector<int> &markerIds) {
    vector<vector<Point2f>> markerCorners;
    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);

    aruco::detectMarkers(image, dictionary, markerCorners, markerIds);

    for (auto const &marker : markerCorners) {
        Point2f min;
        double dist = 999999;

        for (unsigned int i = 0; i < 4; i++) {
            double newDist = sqrt(marker.at(i).x * marker.at(i).x + marker.at(i).y * marker.at(i).y);

            if (newDist < dist) {
                min = marker.at(i);
                dist = newDist;
            }
        }

        corners.emplace_back(min);
    }

    if (spdlog::default_logger()->level() == spdlog::level::debug) {
        stringstream ss;
        ss << "MARKERS RESULT" << endl;
        ss << "MarkersIds: ";
        for (auto const &id : markerIds) ss << to_string(id) << ",";
        ss << endl;
        ss << "MarkersPos: ";
        for (auto const &pos:corners) ss << pos << ",";

        spdlog::debug(ss.str());
    }
}

/**
 * Calcule l'homographie à partir des marqueurs
 * @param markerCorners
 * @param markerIds
 * @param config
 * @param homoMat
 */
void Etallonage::findMarkersHomography(const vector<Point2f> &markerCorners, const vector<int> &markerIds, Mat &homoMat) {
    vector<Point2f> orderMarkersPos;

    for (auto const &id : markerIds) {
        orderMarkersPos.emplace_back(config->markersPos.at(id) * config->boardRatio);
    }

    Mat mat = findHomography(markerCorners, orderMarkersPos);
    mat.copyTo(homoMat);

    spdlog::debug("HOMOGRAPHY RESULT\n {}", mat);
}

/**
 * Trouve la couleur moyenne autour d'un ensemble de points
 * @param image
 * @param pts
 * @return
 */
Scalar Etallonage::readColor(const Mat &image, const vector<Point> &pts) {
    Scalar sum;

    for (auto const &pt : pts) {
        Scalar c = arig_utils::getAverageColor(image, arig_utils::getProbe(pt, config->probeSize));
        sum += arig_utils::ScalarBGR2HSV(c);
    }

    sum /= (float) pts.size();

    return arig_utils::ScalarHSV2BGR(sum);
}

/**
 * Calibre toutes les couleurs
 * @param image
 * @param config
 */
void Etallonage::readColors(const Mat &image) {
    config->blue = readColor(image, config->blueRefs);
    config->red = readColor(image, config->redRefs);
    config->green = readColor(image, config->greenRefs);

    spdlog::debug("COLORS CALIBRATION\n blue: {}\n red: {}\n green: {}", config->blue, config->red, config->green);
}

/**
 * Dessine les coins des marqueurs
 * @param output
 * @param corners
 * @param markerIds
 */
void Etallonage::drawMarkerCorners(Mat &output, const vector<Point2f> &corners, const vector<int> &markerIds) {
    unsigned int i = 0;
    for (auto const &pt : corners) {
        Rect probe = arig_utils::getProbe(pt, 10);
        rectangle(output, probe, Scalar(20, 20, 255));
        string text = "ID:" + to_string(markerIds.at(i++));
        putText(output, text, Point(probe.x + 10, probe.y + 20), 1, 1, Scalar(20, 20, 255), 1);
    }
}

/**
 * Dessine les couleurs trouvés
 * @param output
 * @param config
 */
void Etallonage::drawColors(Mat &output) {
    for (auto const &pt : config->blueRefs) {
        Rect probe = arig_utils::getProbe(pt, config->probeSize);
        rectangle(output, probe, Scalar(20, 255, 20));
    }

    for (auto const &pt : config->redRefs) {
        Rect probe = arig_utils::getProbe(pt, config->probeSize);
        rectangle(output, probe, Scalar(20, 255, 20));
    }

    for (auto const &pt : config->greenRefs) {
        Rect probe = arig_utils::getProbe(pt, config->probeSize);
        rectangle(output, probe, Scalar(20, 255, 20));
    }

    Size cartoucheSize(100, 75);

    Rect bg(output.cols - cartoucheSize.width, output.rows - cartoucheSize.height, cartoucheSize.width,
            cartoucheSize.height);
    rectangle(output, bg, Scalar(255, 255, 255), -1);

    Rect rectBlue(bg.x + 5, bg.y + 5, 10, 10);
    rectangle(output, rectBlue, config->blue, -1);
    putText(output, "blue", Point(rectBlue.x + 15, rectBlue.y + 10), 1, 1, Scalar(0, 0, 0));

    Rect rectRed(bg.x + 5, bg.y + 20, 10, 10);
    rectangle(output, rectRed, config->red, -1);
    putText(output, "red", Point(rectRed.x + 15, rectRed.y + 10), 1, 1, Scalar(0, 0, 0));

    Rect rectGreen(bg.x + 5, bg.y + 35, 10, 10);
    rectangle(output, rectGreen, config->green, -1);
    putText(output, "green", Point(rectGreen.x + 15, rectGreen.y + 10), 1, 1, Scalar(0, 0, 0));
}
