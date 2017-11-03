#include <iostream>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

int main(int argc, char** argv) {
    VideoCapture webCam(0); // Open camera par défaut
    if(!webCam.isOpened())
        return -1;

    Mat frame, grayFrame;
    while (cv::waitKey(30) != 27) {
        // Récupération de l'image
        webCam >> frame;

        // Transformation en gray
        cvtColor(frame, grayFrame, CV_BGR2GRAY);

        // Un petit flou gaussien pour le fun
        GaussianBlur(grayFrame, grayFrame, Size(9, 9), 2 , 2);


        // Application de la detection de cercles
        vector<Vec3f> circles;
        HoughCircles(grayFrame, circles, CV_HOUGH_GRADIENT, 1.2, 50);

        // Dessin des cercles trouvé
        for (size_t i = 0 ; i < circles.size() ; i++) {
            Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);

            circle(frame, center, 3, Scalar(0, 255, 0), -1, 8, 0);
            circle(frame, center, radius, Scalar(0, 0, 255), 3, 8, 0);
        }

        imshow("Result", frame);
    }

    // Le destructeur de VideoCapture fait le job. Pas besoin de delete webCam
    return 0;
}
