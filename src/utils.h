#ifndef VISION_BALISE_UTILS_H
#define VISION_BALISE_UTILS_H

#include "common.h"

namespace arig_utils {
    Scalar ScalarBGR2HSV(Scalar bgr);

    Scalar ScalarHSV2BGR(Scalar hsv);

    void fillRectangle(Mat &mat, Rect &r, Scalar color);

    Rect getProbe(Point pt, int probeSize);

    Scalar getAverageColor(const Mat &mat, Rect r);

    json points2json(vector<Point> points);

    string basename(string const & path);

    string matToBase64(const Mat &mat);
}

#endif //VISION_BALISE_UTILS_H