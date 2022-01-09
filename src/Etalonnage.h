#ifndef VISION_BALISE_ETALONNAGE_H
#define VISION_BALISE_ETALONNAGE_H

#include "common.h"
#include "Config.h"

class Etalonnage {

private:
    Config *config;

    int index = 0;

public:
    explicit Etalonnage(Config *config);
    JsonResult run(const Mat &source);

private:

    bool detectMarkers(const Mat &source, Mat &output, Point &markerCenter, vector<Point> &markersEchantillons);
    void getClusters(Mat &output, const vector<Point> &markers, vector<vector<Point>> &clusters);
    bool detectPerspectivePoints(const vector<vector<Point>> &clusters, vector<Point2f> &ptsImages, vector<Point2f> &ptsTable);
};

#endif //VISION_BALISE_ETALONNAGE_H
