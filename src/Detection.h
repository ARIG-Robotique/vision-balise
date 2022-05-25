#ifndef VISION_BALISE_DETECTION_H
#define VISION_BALISE_DETECTION_H

#include "common.h"
#include "Config.h"

struct Echantillon {
    string c;
    int x;
    int y;
};

class Detection {

private:
    Config* config;

    int index = 0;

public:
    explicit Detection(Config* config);
    json run(const Mat &source);

private:
    void detectMarkers(const Mat &source, Mat &output, vector<Echantillon> &echantillons);
    void detectDistribs(const Mat &source, Mat &output, vector<string> &distribs);
};

#endif //VISION_BALISE_DETECTION_H
