#include <ctime>
#include <csignal>
#include "spdlog/sinks/basic_file_sink.h"
#include "common.h"
#include "Calibration.h"
#include "MultiSocketHelper.h"
#include "ProcessThread.h"
#include "test.h"

string getTimeStr() {
    time_t now = time(nullptr);
    tm *ptm = localtime(&now);
    char timeBuffer[15];
    strftime(timeBuffer, 15, "%Y%m%d%H%M%S", ptm);
    return string(timeBuffer);
}

std::function<void(int)> shutdown_handler;

int main(int argc, char **argv) {
    const String keys =
            "{help | | Display help }"
            "{test | | Run in test mode }"
            "{debug | | Debug messages }"
            "{output-dir | logs/ | Process images storage }"
            "{calibration | | Run calibration }"
            "{calibration-dir | samples/calib/ | Calibration images }"
            "{calibration-file | calibration.yml | Calibration file }"
            "{config-file | config.yml | Config file }"
            "{socket-type | inet | Socket type (inet or unix) }"
            "{socket-port | 9042 | Port for inet socket }"
            "{socket-file | /tmp/vision_balise.sock | File for unix socket }"
            "{mock-photo | | Camera mock file }";

    CommandLineParser parser(argc, argv, keys);
    parser.about("ARIG Vision Balise 2022");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    // dossier de stockage des logs/images
    const String outputDir = parser.get<string>("output-dir");
    const String outputPrefix = outputDir + getTimeStr();

    mkdir(outputDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir(outputPrefix.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // configuration du logger
    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(outputPrefix + "-traces.log");
    spdlog::default_logger()->sinks().push_back(file_sink);
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_level(spdlog::level::debug);

    // démarrage
    spdlog::info("Start vision balise");
    spdlog::debug("Output prefix is {}", outputPrefix.c_str());

    // configuration globale
    Config config;
    config.debug = parser.has("debug") || parser.has("test");
    config.testMode = parser.has("test");
    config.outputPrefix = outputPrefix;

    const String configFilename = parser.get<String>("config-file");
    const String calibFilename = parser.get<String>("calibration-file");
    const String calibDir = parser.get<String>("calibration-dir");

    // lecture de la config
    if (!config.readConfigFile(configFilename)) {
        spdlog::error("Unable to load configuration");
        return 2;
    }

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

    if (parser.has("mock-photo")) {
        config.mockPhoto = parser.get<string>("mock-photo");
    }

    // création du thread de processing
    ProcessThread processThread(&config);
    if (!processThread.isReady()) {
        spdlog::error("Cannot create process thread");
        return 2;
    }

    // mode de test
    if (config.testMode) {
        config.undistort = false;
        if (processThread.takePhoto()) {
            runTest(processThread.getImgOrig(), &config);
            return 0;
        } else {
            return 2;
        }
    }

    // ouverture de la socket
    if (parser.get<string>("socket-type") != "inet") {
        spdlog::error("Seule les socket INET sont supportées");
        return 2;
    }

    MultiSocketHelper socket(parser.get<int>("socket-port"));

    socket.init();
    processThread.setIdle();

    // boucle de commande
    bool stop = false;

    shutdown_handler = [&](int v) mutable {
        processThread.exit();
        socket.end();
        exit(v);
    };
    signal(SIGTERM, [](int v) {
        shutdown_handler(v);
    });

    while (!stop) {
        JsonQuery query = socket.waitQuery();
        if (query.action == DATA_INVALID) {
            spdlog::warn("Données invalides");
            continue;
        }

        JsonResult result;
        if (query.action == ACTION_ECHO) {
            result.status = RESPONSE_OK;
            result.data = query.data;

        } else if (query.action == ACTION_EXIT) {
            spdlog::info("Demande d'arret du programe");
            result.status = RESPONSE_OK;
            stop = true;

        } else if (query.action == ACTION_STATUS) {
            result = processThread.getStatus();

        } else if (query.action == ACTION_PHOTO) {
            result = processThread.getPhoto();

        } else if (query.action == ACTION_IDLE) {
            result = processThread.setIdle();

        } else if (query.action == ACTION_ETALONNAGE) {
            result = processThread.startEtalonnage();

        } else if (query.action == ACTION_DETECTION) {
            result = processThread.startDetection();

        } else {
            spdlog::warn("Action {} non supportée", query.action);
            result.status = RESPONSE_ERROR;
            result.errorMessage = "Action non supportée";
        }

        result.action = query.action;
        socket.sendResponse(query.socket, result);
    }

    processThread.exit();
    socket.end();

    return 0;
}
