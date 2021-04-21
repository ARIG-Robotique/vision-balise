#ifndef VISION_BALISE_COMMON_H
#define VISION_BALISE_COMMON_H

#include <iostream>
#include <math.h>
#include "opencv2/opencv.hpp"
#include "opencv2/aruco.hpp"
#include "json.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

using namespace cv;
using namespace std;
using json = nlohmann::json;

#define DATA_INVALID    "DATA_INVALID"
#define DATA_UNPARSABLE "DATA_UNPARSABLE"

#define RESPONSE_OK     "OK"
#define RESPONSE_ERROR  "ERROR"

#define ACTION_EXIT "EXIT"
#define ACTION_ECHO "ECHO"
#define ACTION_STATUS "STATUS"
#define ACTION_PHOTO "PHOTO"
#define ACTION_IDLE "IDLE"
#define ACTION_DETECTION "DETECTION"
#define ACTION_ETALONNAGE "ETALONNAGE"

#define COLOR_RED "RED"
#define COLOR_GREEN "GREEN"
#define COLOR_UNKNOWN "UNKNOWN"

#define TEAM_BLEU "BLEU"
#define TEAM_JAUNE "JAUNE"
#define TEAM_UNKNOWN "UNKNOWN"

#define DIR_UP "UP"
#define DIR_DOWN "DOWN"
#define DIR_UNKNOWN "UNKNOWN"

#define BOUE_PRESENT "PRESENT"
#define BOUE_ABSENT "ABSENT"

struct JsonResult {
    string status;
    string action;
    string errorMessage;
    json data;
};

struct JsonQuery {
    string action;
    json data;
};

#endif //VISION_BALISE_COMMON_H
