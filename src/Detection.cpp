#include "Detection.h"
#include "utils.h"

Detection::Detection(Config *config) {
    this->config = config;
}

json Detection::run(const Mat &source, int index) {
    spdlog::info("Detection {}", index);

    json r;

    // correction de l'objectif
    Mat undistorted;
    undistort(source, undistorted, config->cameraMatrix, config->distCoeffs);

    // correction de la perspective
    warpPerspective(undistorted, undistorted, config->homography,
                    Size(config->boardSize.width * config->boardRatio, config->boardSize.height * config->boardRatio));

    // detection
    vector<Point> foundBlue, verifiedBlue;
    detectColor(undistorted, config->blue, config->blueRefs, foundBlue, verifiedBlue);

    vector<Point> foundRed, verifiedRed;
    detectColor(undistorted, config->red, config->redRefs, foundRed, verifiedRed);

    vector<Point> foundGreen, verifiedGreen;
    detectColor(undistorted, config->green, config->greenRefs, foundGreen, verifiedGreen);

    if (config->debug) {
        drawObjects(undistorted, foundBlue, "Blue", config->blue);
        drawObjects(undistorted, foundRed, "Red", config->red);
        drawObjects(undistorted, foundGreen, "Green", config->green);

        drawObjects(undistorted, verifiedBlue, "Still blue", config->blue);
        drawObjects(undistorted, verifiedRed, "Still red", config->red);
        drawObjects(undistorted, verifiedGreen, "Still green", config->green);

        imwrite(config->outputPrefix + "detection-" + to_string(index) + ".jpg", undistorted);
    }

    if (config->testMode) {
        imshow("Process", undistorted);
        waitKey(0);
    }

    r["foundBlue"] = arig_utils::points2json(foundBlue);
    r["foundRed"] = arig_utils::points2json(foundRed);
    r["foundGreen"] = arig_utils::points2json(foundGreen);
    r["verifiedBlue"] = arig_utils::points2json(verifiedBlue);
    r["verifiedRed"] = arig_utils::points2json(verifiedRed);
    r["verifiedGreen"] = arig_utils::points2json(verifiedGreen);

    spdlog::debug("DETECTION RESULT\n {}", r.dump(2));

    return r;
}

/**
 * Retourne une plage de couleur pour le seuillage
 * @param color
 * @return
 */
vector<Scalar> Detection::getColorRange(const Scalar &color) {
    return {
            arig_utils::ScalarBGR2HSV(color) - Scalar(30, 50, 50),
            arig_utils::ScalarBGR2HSV(color) + Scalar(30, 50, 50)
    };
}

/**
 * Retourne une plage de surface pour le filtrage
 * @param diametreObjet
 * @param boardRatio
 * @return
 */
vector<double> Detection::getAreaRange(const int diametreObjet, const float boardRatio) {
    double surfaceObjet = CV_PI * diametreObjet * diametreObjet / 4;
    return {
            surfaceObjet * boardRatio * 0.8,
            surfaceObjet * boardRatio * 1.6
    };
}

/**
 * Detecte les blobs d'une couleur
 * @param image
 * @param color
 * @param yCorrection
 * @param foundObjects
 */
void Detection::detectColor(const Mat &image, const Scalar &color, const vector<Point> &refs, vector<Point> &foundObjects, vector<Point> &verifiedObjects) {
    Mat imageHsv, imageThreshold;
    cvtColor(image, imageHsv, COLOR_BGR2HSV);

    vector<Scalar> colorRange = getColorRange(color);
    inRange(imageHsv, colorRange[0], colorRange[1], imageThreshold);

    vector<vector<Point> > contours;
    findContours(imageThreshold, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    vector<double> areaRange = getAreaRange(config->diametreObjet, config->boardRatio);

    for (unsigned int index = 0; index < contours.size(); index++) {
        Moments moment = moments(contours[index]);
        double area = moment.m00;

        if (area > areaRange[0] && area < areaRange[1]) {
            float x = moment.m10 / area;
            float y = moment.m01 / area + config->yCorrection;
            Point2f center(x, y);

            bool inZone = false;

            for (auto const &zone : config->detectZones) {
                if (zone.contains(center)) {
                    inZone = true;
                    break;
                }
            }

            if (inZone) {
                foundObjects.emplace_back(center);
            }
            else {
                for (const auto &ref : refs) {
                    if (pointPolygonTest(contours[index], ref, false) > 0) {
                        verifiedObjects.emplace_back(ref);
                    }
                }
            }
        }
    }
}

/**
 * Dessine les blobs
 * @param output
 * @param objects
 * @param name
 * @param color
 */
void Detection::drawObjects(const Mat &output, const vector<Point> &objects, const String &name, const Scalar &color) {
    for (auto const &pt : objects) {
        circle(output, pt, 20, Scalar(255, 20, 20));
        putText(output, name, pt + Point(35, 35), 1, 1, color, 1);
    }
}

/**
 * Demo de inRange/dilate/erode
 * @param img
 * @param range
 */
void interractiveThreshold(const Mat &img, const vector<Scalar> &range) {
    const String winname = "Settings";
    namedWindow(winname);

    int low_H = range[0][0], low_S = range[0][1], low_V = range[0][2];
    int high_H = range[1][0], high_S = range[1][1], high_V = range[1][2];
    int erode_Q = 2, dilate_Q = 2;

    createTrackbar("Low H", winname, &low_H, 180);
    createTrackbar("High H", winname, &high_H, 180);
    createTrackbar("Low S", winname, &low_S, 255);
    createTrackbar("High S", winname, &high_S, 255);
    createTrackbar("Low V", winname, &low_V, 255);
    createTrackbar("High V", winname, &high_V, 255);
    createTrackbar("Erode", winname, &erode_Q, 5);
    createTrackbar("Dilate", winname, &dilate_Q, 5);

    Mat imgHsv, imgThres;
    cvtColor(img, imgHsv, COLOR_BGR2HSV);

    while (true) {
        inRange(imgHsv, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), imgThres);
        dilate(imgThres, imgThres, Mat(), Point(-1, -1), dilate_Q);
        erode(imgThres, imgThres, Mat(), Point(-1, -1), erode_Q);

        imshow("Process", imgThres);

        char key = (char) waitKey(30);
        if (key == 27) {
            break;
        }
    }
}

/**
 * Demo de SimpleBlobDetector
 * @param image
 */
void interractiveBlobs(const Mat &image) {
    const String winname = "Settings";
    namedWindow(winname);

    SimpleBlobDetector::Params defaultParams;

    int minRepeatability = defaultParams.minRepeatability, minDistBetweenBlobs = defaultParams.minDistBetweenBlobs;
    int thresholdStep = defaultParams.thresholdStep, minThreshold = defaultParams.minThreshold, maxThreshold = defaultParams.maxThreshold;

    int filterByArea = 1, minArea = defaultParams.minArea, maxArea = 1000;
    int filterByCircularity = 1, minCircularity = defaultParams.minCircularity * 10, maxCircularity = 100;
    int filterByInertia = 1, minInertiaRatio = defaultParams.minInertiaRatio * 10, maxInertiaRatio = 100;
    int filterByConvexity = 1, minConvexity = defaultParams.minConvexity * 10, maxConvexity = 100;

    createTrackbar("Min repeatability", winname, &minRepeatability, 10);
    createTrackbar("Min dist between blobs", winname, &minDistBetweenBlobs, 100);
    createTrackbar("Threshold step", winname, &thresholdStep, 100);
    createTrackbar("Min threshold", winname, &minThreshold, 100);
    createTrackbar("Max threshold", winname, &maxThreshold, 100);
    createTrackbar("Filter by area", winname, &filterByArea, 1);
    createTrackbar("Min area", winname, &minArea, 1000);
    createTrackbar("Max area", winname, &maxArea, 1000);
    createTrackbar("Filter by circularity", winname, &filterByCircularity, 1);
    createTrackbar("Min circularity", winname, &minCircularity, 100);
    createTrackbar("Max circularity", winname, &maxCircularity, 100);
    createTrackbar("Filter by inertia", winname, &filterByInertia, 1);
    createTrackbar("Min inertia", winname, &minInertiaRatio, 100);
    createTrackbar("Max inertia", winname, &maxInertiaRatio, 100);
    createTrackbar("Filter by convexity", winname, &filterByConvexity, 1);
    createTrackbar("Min convexity", winname, &minConvexity, 100);
    createTrackbar("Max convexity", winname, &maxConvexity, 100);

    while (true) {
        SimpleBlobDetector::Params params;
        params.minRepeatability = minRepeatability;
        params.minDistBetweenBlobs = minDistBetweenBlobs;
        params.thresholdStep = thresholdStep;
        params.minThreshold = minThreshold;
        params.maxThreshold = maxThreshold;
        params.filterByArea = filterByArea == 1;
        params.minArea = minArea;
        params.maxArea = maxArea;
        params.filterByCircularity = filterByCircularity == 1;
        params.minCircularity = minCircularity / 10;
        params.maxCircularity = maxCircularity / 10;
        params.filterByInertia = filterByInertia == 1;
        params.minInertiaRatio = minInertiaRatio / 10;
        params.maxInertiaRatio = maxInertiaRatio / 10;
        params.filterByConvexity = filterByConvexity == 1;
        params.minConvexity = minConvexity / 10;
        params.minConvexity = minConvexity / 10;
        params.filterByColor = false;

        Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
        vector<KeyPoint> keypoints;
        detector->detect(image, keypoints);

        Mat output = image.clone();

        for (auto const &pos : keypoints) {
            circle(output, pos.pt, pos.size / 2, Scalar(255, 20, 20));
        }

        imshow("Process", output);

        char key = (char) waitKey(30);
        if (key == 27) {
            break;
        }
    }
}
