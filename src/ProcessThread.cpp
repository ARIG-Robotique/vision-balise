#include <thread>
#include "ProcessThread.h"
#include "Detection.h"
#include "Etalonnage.h"
#include "utils.h"

ProcessThread::ProcessThread(Config *config) {
    m_config = config;
    m_action = ACTION_IDLE;

    if (!m_config->mockPhoto.empty()) {
        m_ready = pthread_create(&m_thread, nullptr, &ProcessThread::create, this) != -1;
    } else {
        m_videoThread = new VideoThread(config);

        if (m_videoThread->waitReady()) {
            m_ready = pthread_create(&m_thread, nullptr, &ProcessThread::create, this) != -1;
        }
    }
}

//////////////////////
// THREAD PRINCIPAL //
//////////////////////

/**
 * Retourne si le sous process a bien été lancé
 * @return
 */
bool ProcessThread::isReady() {
    return m_ready;
}

/**
 * Retourne l'état de l'étoallonage et de la détection
 * @return
 */
JsonResult ProcessThread::getStatus() {
    json datas;

    pthread_mutex_lock(&m_datasMutex);
    datas["cameraReady"] = true;
    datas["detection"] = m_detectionResult;
    pthread_mutex_unlock(&m_datasMutex);

    JsonResult r;
    r.status = RESPONSE_OK;
    r.datas = datas;
    return r;
}

/**
 * Retourne la dernière photo en base64
 * @return
 */
JsonResult ProcessThread::getPhoto() {
    json datas;

    pthread_mutex_lock(&m_datasMutex);
    if (!m_imgOrig.empty()) {
        datas = arig_utils::matToBase64(m_imgOrig);
    }
    pthread_mutex_unlock(&m_datasMutex);

    JsonResult r;

    if (datas.empty()) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Pas d'image";
    } else {
        r.status = RESPONSE_OK;
        r.datas = datas;
    }

    return r;
}

JsonResult ProcessThread::setIdle() {
    return action(ACTION_IDLE);
}

/**
 * Affiche al dernière image dans uen fenêtre
 */
void ProcessThread::displayPhoto() {
    pthread_mutex_lock(&m_datasMutex);
    if (!m_imgOrig.empty()) {
        imshow("Photo", m_imgOrig);
        waitKey(1);
    } else {
        spdlog::warn("Photo is empty");
    }
    pthread_mutex_unlock(&m_datasMutex);
}

/**
 * Démarre la détection dans le sous process
 * @return
 */
JsonResult ProcessThread::startDetection() {
    return action(ACTION_DETECTION);
}

/**
 * Démarre l'étalonnage dans le sous process
 * @return
 */
JsonResult ProcessThread::startEtalonnage() {
    action(ACTION_IDLE);

    Etalonnage etalonnage(m_config);

    JsonResult r;

    if (takePhoto("etalonnage")) {
        r.status = RESPONSE_OK;
        r.datas = etalonnage.run(m_imgOrig);
    } else {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de prendre une photo";
    }

    return r;
}

/**
 * Demande au sous process de s'arreter
 * @return
 */
JsonResult ProcessThread::exit() {
    return action(ACTION_EXIT);
}

/**
 * Notifie le sous process que l'action à changé
 * @param _action
 * @return
 */
JsonResult ProcessThread::action(const char *_action) {
    spdlog::info("ProcessThread: Action {}", _action);

    pthread_mutex_lock(&m_actionMutex);
    m_action = string(_action);
    pthread_mutex_unlock(&m_actionMutex);

    JsonResult r;
    r.status = RESPONSE_OK;
    return r;
}

/**
 * Lance le sous process vers soi-même
 * @param context
 * @return
 */
void *ProcessThread::create(void *context) {
    return ((ProcessThread *) context)->process();
}


///////////////////////
// THREAD SECONDAIRE //
///////////////////////

/**
 * Boucle principale du sous process
 * @return
 */
void *ProcessThread::process() {
    spdlog::info("ProcessThread: ready");

    bool stop = false;
    string action;

    while (!stop) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action == ACTION_EXIT) {
            if (m_videoThread != nullptr) {
                m_videoThread->exit();
            }
            spdlog::info("ProcessThread: Demande d'arret du thread");
            stop = true;

        } else if (action == ACTION_IDLE) {
            spdlog::info("ProcessThread: Démarrage de la prise de photo");
            processIdle();

        } else if (action == ACTION_DETECTION) {
            spdlog::info("ProcessThread: Démarrage de la détection");
            processDetection();

        } else {
            spdlog::warn("ProcessThread: action non supportée");
        }
    }

    pthread_exit(nullptr);
}

bool ProcessThread::takePhoto(const string &name) {
    if (!m_config->mockPhoto.empty()) {
        pthread_mutex_lock(&m_datasMutex);
        m_imgOrig = imread(m_config->mockPhoto, CV_LOAD_IMAGE_COLOR);
        pthread_mutex_unlock(&m_datasMutex);

        if (!m_imgOrig.data) {
            spdlog::error("Could not open or find the mock image");
            return false;
        } else {
            return true;
        }
    }

    Mat source = m_videoThread->getPhoto();

    if (!source.data) {
        spdlog::error("Could not open or find the camera");
        return false;
    } else {
        Mat undistorted;
        if (m_config->undistort) {
            undistort(source, undistorted, m_config->cameraMatrix, m_config->distCoeffs);
        } else {
            undistorted = source.clone();
        }

        Mat final;
        if (m_config->swapRgb) {
            cvtColor(undistorted, final, COLOR_RGB2BGR);
        } else {
            final = undistorted;
        }

        pthread_mutex_lock(&m_datasMutex);
        m_imgOrig = final;
        pthread_mutex_unlock(&m_datasMutex);

        return true;
    }
}

/**
 * Boucle de photo
 */
void ProcessThread::processIdle() {
    int i = 1;
    string action;

    while (true) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action != ACTION_IDLE) {
            break;
        }

        auto start = arig_utils::startTiming();
        if (takePhoto("idle-" + to_string(i))) {
            spdlog::info("Photo took in {}ms", arig_utils::ellapsedTime(start));
        }

        i++;

        this_thread::sleep_for(chrono::seconds(1));
    }
}

/**
 * Boucle de détection
 */
void ProcessThread::processDetection() {
    Detection detection(m_config);

    int i = 0;
    string action;

    while (true) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action != ACTION_DETECTION) {
            break;
        }

        if (takePhoto("detection-" + to_string(i))) {
            json r = detection.run(m_imgOrig, i);

            pthread_mutex_lock(&m_datasMutex);
            m_detectionResult = r;
            pthread_mutex_unlock(&m_datasMutex);
        }

        i++;

        this_thread::sleep_for(chrono::seconds(1));
    }
}
