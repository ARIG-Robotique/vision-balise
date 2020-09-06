#ifndef VISION_BALISE_DETECTION_H
#define VISION_BALISE_DETECTION_H

#include "common.h"
#include "Config.h"

class Detection {

private:
    Config* config;

    short bufferIndex = 0;
    vector<vector<int>> ecueilBuffer;
    vector<vector<int>> boueesBuffer;

public:
    explicit Detection(Config* config);
    json run(const Mat &source, int index);

private:
    void findMarkers(const Mat &image, vector<vector<Point2f>> &markerCorners, vector<int> &markerIds);
    vector<Point2f> getMarkerById(vector<vector<Point2f>> &markerCorners, vector<int> &markerIds, int id);
    bool isMarkerUpside(vector<Point2f> &marker);
    vector<string> readColorsEcueil(const Mat &image);
    vector<string> checkPresenceBouees(const Mat &image);
};


#endif //VISION_BALISE_DETECTION_H
