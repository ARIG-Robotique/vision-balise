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
    bool detectMarker(const Mat &source, Mat &output, Point &pt);

    bool calibCouleurs(const Mat &source, Mat &output, const Point &markerCenter);

    bool calculPerspective(const Mat &source, Mat &output, const Point &markerCenter);

    bool detectBouees(const Mat &labA, Mat &output, const Point &markerCenter,
                      const vector<double> &colorRange, const vector<Point> &detectionZone, bool sideIsMinX,
                      Point &boueeTop, Point &boueeSide);

    bool calibCouleursEcueil(const Mat &source, Mat &output);

    void debugResult(Mat &output);
};


#endif //VISION_BALISE_ETALONNAGE_H
