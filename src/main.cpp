#include <ctime>
#include "common.h"
#include "Calibration.h"
#include "SocketHelper.h"
#include "ProcessThread.h"
#include "tester.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "utils.h"

int main(int argc, char **argv) {
    const String keys =
            "{help | | Display help }"
            "{test | | Run in test mode }"
            "{debug | | Debug messages }"
            "{output-dir | output/ | Process images storage }"
            "{calibration | | Run calibration }"
            "{calibration-dir | samples/calib/ | Calibration images }"
            "{calibration-file | calibration.yml | Calibration file }"
            "{config-file | config.yml | Config file }"
            "{socket-type | inet | Socket type (inet or unix) }"
            "{socket-port | 9042 | Port for inet socket }"
            "{socket-file | /tmp/vision_balise.sock | File for unix socket }";

    CommandLineParser parser(argc, argv, keys);
    parser.about("ARIG Vision Balise 2020");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    // construit la date d'execution
    time_t now = time(nullptr);
    tm *ptm = localtime(&now);
    char timeBuffer[15];
    strftime(timeBuffer, 15, "%Y%m%d%H%M%S", ptm);

    const String outputDir = parser.get<string>("output-dir");
    const String outputPrefix = outputDir + String(timeBuffer) + "-";

    // configuration du logger
    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(outputPrefix + "log");
    spdlog::default_logger()->sinks().push_back(file_sink);
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_level(parser.has("debug") ? spdlog::level::debug : spdlog::level::info);

    // démarrage
    spdlog::info("Start vision balise");
    spdlog::debug("Output prefix is {}", outputPrefix.c_str());

    // configuration globale
    Config config;
    config.debug = parser.has("debug") || parser.has("test");
    config.testMode = parser.has("test");
    config.outputDir = outputDir;
    config.outputPrefix = outputPrefix;

    const String configFilename = parser.get<String>("config-file");
    const String calibFilename = parser.get<String>("calibration-file");
    const String calibDir = parser.get<String>("calibration-dir");

    // calibration
    if (parser.has("calibration")) {
        Calibration calibration;

        if (calibration.runAndSave(calibDir, calibFilename, &config)) {
            return 0;
        }
        return 2;
    }

    // lecture de la calibration
    if (!config.readCalibrationFile(calibFilename)) {
        spdlog::warn("Not calibrated, please run --calibration");
        return 2;
    }

    // lecture de la config
    if (!config.readConfigFile(configFilename)) {
        spdlog::error("Unable to load configuration");
        return 2;
    }

    // mode de test
    if (config.testMode) {
        runTest(&config);
        return 0;
    }

    // ouverture de la socket
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

    // création du thread de processing
    ProcessThread processThread(&config);
    if (!processThread.isReady()) {
        spdlog::error("Cannot create OpenCV thread");
        return 2;
    }

    // boucle de commande
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
            int width = 1296;
            if (query.datas["width"] != nullptr) {
                width = query.datas["width"].get<int>();
            }
            result = processThread.getPhoto(width);

        } else if (query.action == ACTION_ETALONNAGE) {
            if (query.datas["ecueil"] == nullptr || query.datas["ecueil"].size() != 2 ||
                    (query.datas["bouees"] != nullptr && query.datas["bouees"].size() != 8)) {
                result.status = RESPONSE_ERROR;
                result.errorMessage = "Invalid points";
            } else {
                config.ecueil = arig_utils::json2points(query.datas["ecueil"]);

                if (query.datas["bouees"] != nullptr) {
                    config.bouees = arig_utils::json2points(query.datas["bouees"]);
                } else {
                    config.bouees.clear();
                }

                result = processThread.startEtalonnage();
            }

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
