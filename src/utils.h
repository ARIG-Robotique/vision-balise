#ifndef VISION_BALISE_UTILS_H
#define VISION_BALISE_UTILS_H

#include "common.h"

namespace arig_utils {
    Scalar ScalarBGR2HSV(Scalar bgr);

    Scalar ScalarHSV2BGR(Scalar hsv);

    void fillRectangle(Mat &mat, Rect &r, Scalar color);

    Rect getProbe(Point pt, int probeSize);

    Scalar getAverageColor(const Mat &mat, Rect r);

    json points2json(vector<Point> &points);

    vector<Point> json2points(json &data);

    json scalar2json(Scalar scalar);

    json scalars2json(vector<Scalar> &scalars);

    string basename(string const & path);

    string matToBase64(const Mat &mat);

    chrono::high_resolution_clock::time_point startTiming();

    long ellapsedTime(chrono::high_resolution_clock::time_point start);
}

#endif //VISION_BALISE_UTILS_H