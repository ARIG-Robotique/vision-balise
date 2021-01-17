#include "utils.h"
#include "base64.h"

namespace arig_utils {
    extern const Scalar RED = Scalar(0, 0, 255);
    extern const Scalar GREEN = Scalar(0, 255, 0);
    extern const Scalar BLUE = Scalar(255, 0, 0);
    extern const Scalar WHITE = Scalar(255, 255, 255);
    extern const Scalar BLACK = Scalar(0, 0, 0);

    Scalar ScalarBGR2HSV(const Scalar &bgr) {
        Mat dst;
        Mat src(1, 1, CV_8UC3, bgr);
        cvtColor(src, dst, CV_BGR2HSV);
        return Scalar(dst.data[0], dst.data[1], dst.data[2]);
    }

    Scalar ScalarHSV2BGR(const Scalar &hsv) {
        Mat dst;
        Mat src(1, 1, CV_8UC3, hsv);
        cvtColor(src, dst, CV_HSV2BGR);
        return Scalar(dst.data[0], dst.data[1], dst.data[2]);
    }

    void fillRectangle(Mat &mat, const Rect &r, const Scalar &color) {
        Point points[1][4];

        points[0][0] = Point(r.x, r.y);
        points[0][1] = Point(r.x + r.width, r.y);
        points[0][2] = Point(r.x + r.width, r.y + r.height);
        points[0][3] = Point(r.x, r.y + r.height);

        int size = 4;

        const Point *points_[1] = {points[0]};

        fillPoly(mat, points_, &size, 1, color);
    }

    Rect getProbe(const Point &pt, int probeSize) {
        Rect rect;
        rect.x = pt.x - probeSize / 2;
        rect.y = pt.y - probeSize / 2;
        rect.width = probeSize;
        rect.height = probeSize;
        return rect;
    }

    Scalar getAverageColor(const Mat &mat, const Rect &r) {
        Mat1b mask(mat.rows, mat.cols, uchar(0));
        fillRectangle(mask, r, Scalar(255));

        return mean(mat, mask);
    }

    json points2json(const vector<Point> &points) {
        json arr;

        for (auto &pt : points) {
            arr.emplace_back(json({pt.x, pt.y}));
        }

        return arr;
    }

    json strings2json(const vector<string> &values) {
        json arr;

        for (auto &str : values) {
            arr.emplace_back(str);
        }

        return arr;
    }

    vector<Point> json2points(const json &data) {
        vector<Point> points;

        for (auto &pt : data) {
            points.emplace_back(Point(pt[0].get<int>(), pt[1].get<int>()));
        }

        return points;
    }

    json scalar2json(const Scalar &scalar) {
        return json({scalar[0], scalar[1], scalar[2]});
    }

    json scalars2json(const vector<Scalar> &scalars) {
        json arr;

        for (auto &scalar : scalars) {
            arr.emplace_back(scalar2json(scalar));
        }

        return arr;
    }

    string basename(const string &path) {
        return path.substr(path.find_last_of("/\\") + 1);
    }

    string matToBase64(const Mat &mat) {
        vector<uchar> buf;
        imencode(".jpg", mat, buf);
        uchar *enc_msg = new uchar[buf.size()];
        for (int i = 0; i < buf.size(); i++) enc_msg[i] = buf[i];
        string encoded = base64_encode(enc_msg, buf.size());
        return encoded;
    }

    chrono::high_resolution_clock::time_point startTiming() {
        return chrono::high_resolution_clock::now();
    }

    long ellapsedTime(chrono::high_resolution_clock::time_point start) {
        auto elapsed = chrono::high_resolution_clock::now() - start;
        return chrono::duration_cast<chrono::milliseconds>(elapsed).count();
    }

    vector<Point> pointsOfMaxX(const vector<Point> &points) {
        vector<Point> result;
        int maxX = 0;

        for (auto &point : points) {
            if (point.x > maxX) {
                result.clear();
                maxX = point.x;
            }
            if (point.x == maxX) {
                result.emplace_back(point);
            }
        }

        return result;
    }

    vector<Point> pointsOfMinX(const vector<Point> &points) {
        vector<Point> result;
        int minX = 9999999;

        for (auto &point : points) {
            if (point.x < minX) {
                result.clear();
                minX = point.x;
            }
            if (point.x == minX) {
                result.emplace_back(point);
            }
        }

        return result;
    }

    vector<Point> pointsOfMaxY(const vector<Point> &points) {
        vector<Point> result;
        int maxY = 0;

        for (auto &point : points) {
            if (point.y > maxY) {
                result.clear();
                maxY = point.y;
            }
            if (point.y == maxY) {
                result.emplace_back(point);
            }
        }

        return result;
    }

    double averageX(const vector<Point> &points) {
        double sum = 0;

        for (auto &point : points) {
            sum += point.x;
        }

        return sum / points.size();
    }

    double averageY(const vector<Point> &points) {
        double sum = 0;

        for (auto &point : points) {
            sum += point.y;
        }

        return sum / points.size();
    }

    Point tablePtToImagePt(const Point &pt) {
        return Point((3000 - pt.x) / 2.0, (2000 - pt.y) / 2.0);
    }

    Point imagePtToTablePt(const Point &pt) {
        return Point(3000 - pt.x * 2, 2000 - pt.y * 2);
    }

    /**
     * inRange compatible une plage de H qui croise l'origine (0=180)
     */
    void hsvInRange(const Mat &imageHsv, const vector<Scalar> &range, Mat &output) {
        assert(range.size() == 2);

        if (range[0][0] <= range[1][0]) {
            inRange(imageHsv, range[0], range[1], output);
        } else {
            Mat out1, out2;
            inRange(imageHsv, range[0], Scalar(180, range[1][1], range[1][2]), out1);
            inRange(imageHsv, Scalar(0, range[0][1], range[0][2]), range[1], out2);
            output = out1 | out2;
        }
    }
}