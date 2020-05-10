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
    void findMarkers(const Mat &image, vector<vector<Point2f>> &markerCorners, vector<int> &markerIds);

    vector<Point2f> getMarkerById(vector<vector<Point2f>> &markerCorners, vector<int> &markerIds, int id);

    bool isMarkerUpside(vector<Point2f> &marker);

    vector<string> readColorsEcueil(const Mat &image);
    vector<string> readColors(const Mat &image, const vector<Point> &points);
};


#endif //VISION_BALISE_DETECTION_H
