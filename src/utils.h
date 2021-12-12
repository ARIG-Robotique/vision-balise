#ifndef VISION_BALISE_UTILS_H
#define VISION_BALISE_UTILS_H

#include "common.h"

namespace arig_utils {
    extern const Scalar RED;
    extern const Scalar GREEN;
    extern const Scalar BLUE;
    extern const Scalar WHITE;
    extern const Scalar BLACK;

    Scalar ScalarBGR2HSV(const Scalar &bgr);

    Scalar ScalarHSV2BGR(const Scalar &hsv);

    void fillRectangle(Mat &mat, const Rect &r, const Scalar &color);

    Rect getProbe(const Point &pt, int probeSize);

    Scalar getAverageColor(const Mat &mat, const Rect &r);

    json points2json(const vector<Point> &points);

    json strings2json(const vector<string> &values);

    vector<Point> json2points(const json &data);

    json scalar2json(const Scalar &scalar);

    json scalars2json(const vector<Scalar> &scalars);

    String exec(const string &cmd);

    string basename(const string &path);

    string matToBase64(const Mat &mat);

    chrono::high_resolution_clock::time_point startTiming();

    long ellapsedTime(chrono::high_resolution_clock::time_point start);

    vector<Point> pointsOfMaxX(const vector<Point> &points);

    vector<Point> pointsOfMinX(const vector<Point> &points);

    vector<Point> pointsOfMaxY(const vector<Point> &points);

    vector<Point> pointsOfMinY(const vector<Point> &points);

    double averageX(const vector<Point> &points);

    double averageY(const vector<Point> &points);

    Point tablePtToImagePt(const Point &pt);

    Point imagePtToTablePt(const Point &pt);

    void hsvInRange(const Mat &imageHsv, const vector<Scalar> &range, Mat &output);

    Scalar meanBGR(const Scalar &scalar1, const Scalar &scalar2);

    double deltaHue(const Scalar &scalar1, const Scalar &scalar2);
}

#endif //VISION_BALISE_UTILS_H