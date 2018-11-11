#include "Calibration.h"
#include "utils.h"

int checkerSize = 25;
int numBoards = 2;
int numCornersHor = 9;
int numCornersVer = 6;
int numSquares = numCornersHor * numCornersVer;
Size board_sz = Size(numCornersHor, numCornersVer);

/**
 * Lance la calibration et sauvegarde dans un fichier
 * @param directory
 * @param filename
 * @param config
 */
bool Calibration::runAndSave(const String &directory, const String &filename, const Config* config) {
    vector<String> files;
    glob(directory, files);

    Mat cameraMatrix = Mat(3, 3, CV_32FC1);
    Mat distCoeffs;

    if (!run(cameraMatrix, distCoeffs, files, config)) {
        return false;
    }

    FileStorage fs(filename, FileStorage::WRITE);
    fs << "cameraMatrix" << cameraMatrix;
    fs << "distCoeffs" << distCoeffs;
    fs.release();

    return true;
}

/**
 * Calcule les matrices de la caméra à partir de plusieurs photos d'echequier
 * @param cameraMatrix
 * @param distCoeffs
 * @param files
 * @param testMode
 */
bool Calibration::run(Mat &cameraMatrix, Mat &distCoeffs, const vector<String> &files, const Config* config) {
    Size image_sz;

    vector<Point3f> obj;
    for (int j = 0; j < numSquares; j++) {
        obj.emplace_back(Point3f(j / numCornersHor * checkerSize, j % numCornersHor * checkerSize, 0.0f));
    }

    vector<vector<Point3f>> object_points;
    vector<vector<Point2f>> image_points;
    vector<Point2f> corners;
    int successes = 0;

    for (auto const &file : files) {
        Mat image = imread(file, IMREAD_COLOR);
        Mat gray_image;

        cvtColor(image, gray_image, CV_BGR2GRAY);

        bool found = findChessboardCorners(image, board_sz, corners,
                                           CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

        drawChessboardCorners(image, board_sz, corners, found);
        imwrite(config->outputPrefix + "calib-" + arig_utils::basename(file), image);

        if (config->testMode) {
            imshow("Calibration", image);
            waitKey(0);
        }

        if (found) {
            cornerSubPix(gray_image, corners, Size(11, 11), Size(-1, -1),
                         TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));

            image_points.push_back(corners);
            object_points.push_back(obj);
            image_sz = image.size();

            successes++;
        }
    }

    if (successes < numBoards) {
        spdlog::error("Nombre insuffisant d'images de calibration");
        return false;
    }

    vector<Mat> rvecs;
    vector<Mat> tvecs;

    cameraMatrix.ptr<float>(0)[0] = 1;
    cameraMatrix.ptr<float>(1)[1] = 1;

    calibrateCamera(object_points, image_points, image_sz, cameraMatrix, distCoeffs, rvecs, tvecs);

    spdlog::info("CALIBRATION RESULT\n Boards detected {0}/{1}\n cameraMatrix {2}\n distCoeffs {3}", successes, files.size(), cameraMatrix, distCoeffs);

    return true;
}