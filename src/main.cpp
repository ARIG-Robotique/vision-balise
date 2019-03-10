#include <ctime>
#include "common.h"
#include "Calibration.h"
#include "SocketHelper.h"
#include "ProcessThread.h"
#include "tester.h"

int main(int argc, char **argv) {
    const String keys =
            "{help | | Display help }"
            "{test | | Run in test mode }"
            "{debug | | Debug messages }"
            "{output-dir | output/ | Process images storage }"
            "{calibration | | Run calibration }"
            "{calibration-dir | samples/calib/ | Calibration images }"
            "{calibration-file | calibration.yml | Calibration file }"
            "{etallonage | | Run etallonage }"
            "{etallonage-file | | Etallonage file }"
            "{config-file | config.yml | Config file }"
            "{socket-type | inet | Socket type (inet or unix) }"
            "{socket-port | 9042 | Port for inet socket }"
            "{socket-file | /tmp/vision_balise.sock | File for unix socket }";

    CommandLineParser parser(argc, argv, keys);
    parser.about("ARIG Vision Balise v1.0.0");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    spdlog::set_level(parser.has("debug") ? spdlog::level::debug : spdlog::level::info);

    spdlog::info("Start vision balise");

    Config config;
    config.debug = parser.has("debug") || parser.has("test");
    config.testMode = parser.has("test");
    config.outputDir = parser.get<string>("output-dir");

    time_t now = time(nullptr);
    tm *ptm = localtime(&now);
    char timeBuffer[14];
    strftime(timeBuffer, 14, "%Y%m%d%H%M%S", ptm);

    config.outputPrefix = config.outputDir + timeBuffer + "-";
    spdlog::debug("Image output prefix is {}", config.outputPrefix.c_str());

    const String configFilename = parser.get<String>("config-file");
    const String calibFilename = parser.get<String>("calibration-file");
    const String calibDir = parser.get<String>("calibration-dir");
    const String etallonageFilename = parser.get<String>("etallonage-file");

    if (parser.has("calibration")) {
        Calibration calibration;

        if (calibration.runAndSave(calibDir, calibFilename, &config)) {
            return 0;
        }
        return 2;
    }

    if (!config.readCalibrationFile(calibFilename)) {
        spdlog::warn("Not calibrated, please run --calibration");
        return 2;
    }

    if (!config.readConfigFile(configFilename)) {
        spdlog::error("Unable to load configuration");
        return 2;
    }

    if (parser.has("etallonage")) {
        if (etallonageFilename.empty()) {
            spdlog::error("No etallonage file provided");
            return 2;
        }

        Etallonage etallonage(&config);

        if (etallonage.runAndSave(etallonageFilename)) {
            return 0;
        }
        return 2;
    }

    if (config.testMode) {
        runTest(&config);
        return 0;
    }

    if (!etallonageFilename.empty() && !config.readEtallonageFile(etallonageFilename)) {
        spdlog::error("Cannot read provided etallonage file");
        return 2;
    }

    SocketHelper socket(parser.get<string>("socket-type"));
    if (socket.isUnknown()) {
        spdlog::error("Invalid socket type");
        return 2;

    } else if (socket.isInet()) {
        socket.setPort(parser.get<int>("socket-port"));

    } else if (socket.isUnix()) {
        socket.setSocketFile(parser.get<string>("socket-file"));
    }

    socket.init();

    ProcessThread processThread(&config);
    if (!processThread.isReady()) {
        spdlog::error("Cannot create OpenCV thread");
        return 2;
    }

    // etallonage fait depuis un fichier
    if (!etallonageFilename.empty()) {
        spdlog::info("Etallonage from file");
        processThread.setEtallonageOk();
    }

    bool stop = false, waitConnection = true;
    while (!stop) {
        if (waitConnection) {
            spdlog::debug("Attente de connexion client ...");
            socket.waitConnection();
            waitConnection = false;
        }

        JsonQuery query = socket.getQuery();
        if (query.action == DATA_INVALID) {
            spdlog::warn("Données invalide, la socket client est fermé ?");
            waitConnection = true;
            continue;
        }

        JsonResult result;
        if (query.action == ACTION_ECHO) {
            result.status = RESPONSE_OK;
            result.datas = query.datas;

        } else if (query.action == ACTION_EXIT) {
            spdlog::info("Demande d'arret du programe");
            result.status = RESPONSE_OK;
            stop = true;

        } else if (query.action == ACTION_STATUS) {
            result = processThread.getStatus();

        } else if (query.action == ACTION_PHOTO) {
            int width = 1024;
            if (query.datas["width"] != nullptr) {
                width = query.datas["width"].get<int>();
            }
            result = processThread.getPhoto(width);

        } else if (query.action == ACTION_ETALLONAGE) {
            result = processThread.startEtallonage();

        } else if (query.action == ACTION_DETECTION) {
            result = processThread.startDetection();

        } else {
            spdlog::warn("Action {} non supportée", query.action);
            result.status = RESPONSE_ERROR;
            result.errorMessage = "Action non supportée";
        }

        result.action = query.action;
        socket.sendResponse(result);
    }

    processThread.exit();
    socket.end();

    return 0;
}
