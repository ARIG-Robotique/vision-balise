#ifndef VISION_BALISE_ETALLONAGE_H
#define VISION_BALISE_ETALLONAGE_H

#include "common.h"
#include "Config.h"

class Etallonage {
private:
    Config* config;

public:
    explicit Etallonage(Config* config);
    bool run(const Mat &source, int index);

private:
    void findMarkers(const Mat &image, vector<Point2f> &corners, vector<int> &markerIds);
    void findMarkersHomography(const vector<Point2f> &markerCorners, const vector<int> &markerIds, Mat &homoMat);
    Scalar readColor(const Mat &image, const vector<Point> &pts);
    void readColors(const Mat &image);
    void drawMarkerCorners(Mat &output, const vector<Point2f> &corners, const vector<int> &markerIds);
    void drawColors(Mat &output);

};


#endif //VISION_BALISE_ETALLONAGE_H
