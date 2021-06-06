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

    bool calibCouleursEcueil(const Mat &source, Mat &output);

    bool detectBouees(const Mat &imageHsv, Mat &output,
                      const vector<Scalar> &colorRange, const vector<Point> &zone, bool sideIsMinX,
                      Point &boueeTop, Point &boueeSide);

    void debugResult(Mat &output);
};


#endif //VISION_BALISE_ETALONNAGE_H
