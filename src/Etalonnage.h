#ifndef VISION_BALISE_ETALONNAGE_H
#define VISION_BALISE_ETALONNAGE_H

#include "common.h"
#include "Config.h"

class Etalonnage {

private:
    Config* config;

public:
    explicit Etalonnage(Config* config);
    json run(const Mat &source);

private:
    vector<Scalar> readColors(const Mat &source, vector<Point> &points);
    void drawProbes(const Mat &output, vector<Point> &points, vector<Scalar>& colors);
};


#endif //VISION_BALISE_ETALONNAGE_H
