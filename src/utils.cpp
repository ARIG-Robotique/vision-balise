#include "utils.h"
#include "base64.h"

namespace arig_utils {
    Scalar ScalarBGR2HSV(Scalar bgr) {
        Mat dst;
        Mat src(1, 1, CV_8UC3, bgr);
        cvtColor(src, dst, CV_BGR2HSV);
        return Scalar(dst.data[0], dst.data[1], dst.data[2]);
    }

    Scalar ScalarHSV2BGR(Scalar hsv) {
        Mat dst;
        Mat src(1, 1, CV_8UC3, hsv);
        cvtColor(src, dst, CV_HSV2BGR);
        return Scalar(dst.data[0], dst.data[1], dst.data[2]);
    }

    void fillRectangle(Mat &mat, Rect &r, Scalar color) {
        Point points[1][4];

        points[0][0] = Point(r.x, r.y);
        points[0][1] = Point(r.x + r.width, r.y);
        points[0][2] = Point(r.x + r.width, r.y + r.height);
        points[0][3] = Point(r.x, r.y + r.height);

        int size = 4;

        const Point *points_[1] = {points[0]};

        fillPoly(mat, points_, &size, 1, color);
    }

    Rect getProbe(Point pt, int probeSize) {
        Rect rect;
        rect.x = pt.x - probeSize / 2;
        rect.y = pt.y - probeSize / 2;
        rect.width = probeSize;
        rect.height = probeSize;
        return rect;
    }

    Scalar getAverageColor(const Mat &mat, Rect r) {
        Mat1b mask(mat.rows, mat.cols, uchar(0));
        fillRectangle(mask, r, Scalar(255));

        return mean(mat, mask);
    }

    json points2json(vector<Point> &points) {
        json arr;

        for (auto &pt : points) {
            arr.emplace_back(json({ pt.x, pt.y }));
        }

        return arr;
    }

    vector<Point> json2points(json &data) {
        vector<Point> points;

        for (auto &pt : data) {
            points.emplace_back(Point(pt[0].get<int>(), pt[1].get<int>()));
        }

        return points;
    }

    json scalar2json(Scalar scalar) {
        return json({ scalar[0], scalar[1], scalar[2] });
    }

    json scalars2json(vector<Scalar> &scalars) {
        json arr;

        for (auto &scalar : scalars) {
            arr.emplace_back(scalar2json(scalar));
        }

        return arr;
    }

    string basename(string const & path) {
        return path.substr(path.find_last_of("/\\") + 1);
    }

    string matToBase64(const Mat& mat) {
        vector<uchar> buf;
        imencode(".jpg", mat, buf);
        uchar *enc_msg = new uchar[buf.size()];
        for(int i=0; i < buf.size(); i++) enc_msg[i] = buf[i];
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
}