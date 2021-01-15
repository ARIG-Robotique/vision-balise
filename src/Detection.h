#ifndef VISION_BALISE_DETECTION_H
#define VISION_BALISE_DETECTION_H

#include "common.h"
#include "Config.h"

class Detection {

private:
    Config* config;

    short bufferIndex = 0;
    vector<vector<int>> boueesBuffer;

public:
    explicit Detection(Config* config);
    json run(const Mat &source, int index);

private:
    bool lectureGirouette(const Mat &imageHsv, Mat &output, vector<string> &girouette);
    bool lectureEcueil(const Mat &imageHsv, Mat &output, bool adverse, vector<string> &ecueil);
    bool controleBouees(const Mat &imageHsv, Mat &output, vector<string> &bouees);
    bool detectionBouees(const Mat &imageHsv, Mat &output, const vector<Scalar> &colorRange, vector<pair<string, Point>> &bouees);
};


#endif //VISION_BALISE_DETECTION_H
