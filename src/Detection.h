#ifndef VISION_BALISE_DETECTION_H
#define VISION_BALISE_DETECTION_H

#include "common.h"
#include "Config.h"

class Detection {

private:
    Config* config;

public:
    explicit Detection(Config* config);
    json run(const Mat &source, int index);

private:
    vector<Scalar> getColorRange(const Scalar &color);
    vector<double> getAreaRange(const int diametreObjet, const float boardRatio);
    void detectColor(const Mat &image, const Scalar &color, const vector<Point> &refs, vector<Point> &foundObjects, vector<Point> &verifiedObjects);
    void drawObjects(const Mat &output, const vector<Point> &objects, const String &name, const Scalar &color);

};


#endif //VISION_BALISE_DETECTION_H
