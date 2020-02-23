#include <thread>
#include "ProcessThread.h"
#include "Detection.h"
#include "Etalonnage.h"
#include "utils.h"

ProcessThread::ProcessThread(Config *config) {
    m_config = config;
    m_action = ACTION_IDLE;
    m_ready = pthread_create(&m_thread, nullptr, &ProcessThread::create, this) != -1;
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
    datas["cameraReady"] = m_cameraReady;
    datas["detection"] = m_detectionResult;
    datas["etallonage"] = m_etalonnageResult;
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
JsonResult ProcessThread::getPhoto(int width) {
    spdlog::info("Get photo of size {}", width);
    json datas;

    Mat img;
    pthread_mutex_lock(&m_datasMutex);
    if (!m_imgOrig.empty()) {
        Size size(m_imgOrig.size());
        if (size.width > width) {
            size = Size(width, width * size.height / size.width);
        }

        resize(m_imgOrig, img, size);
    }
    pthread_mutex_unlock(&m_datasMutex);

    JsonResult r;

    if (img.empty()) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Pas d'image";
    } else {
        r.status = RESPONSE_OK;
        r.datas = arig_utils::matToBase64(img);
    }

    return r;
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
    return action(ACTION_ETALONNAGE);
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

    m_video = new VideoCapture(m_config->cameraIndex);

    pthread_mutex_lock(&m_datasMutex);
    m_cameraReady = m_video->isOpened();
    pthread_mutex_unlock(&m_datasMutex);

    if (!m_cameraReady) {
        spdlog::error("Cannot open camera");
        pthread_exit(nullptr);
    }

    m_video->set(CV_CAP_PROP_FRAME_WIDTH, m_config->cameraResolution.width);
    m_video->set(CV_CAP_PROP_FRAME_HEIGHT, m_config->cameraResolution.height);

    bool stop = false;
    string action;

    while (!stop) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action == ACTION_EXIT) {
            m_video->release();
            spdlog::info("ProcessThread: Demande d'arret du thread");
            stop = true;

        } else if (action == ACTION_IDLE) {
            spdlog::info("ProcessThread: Démarrage de la prise de photo");
            processIdle();

        } else if (action == ACTION_ETALONNAGE) {
            spdlog::info("ProcessThread: Démarrage de l'étallonage");
            processEtalonnage();

            pthread_mutex_lock(&m_actionMutex);
            action = ACTION_IDLE;
            pthread_mutex_unlock(&m_actionMutex);

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
    Mat source;
    m_video->read(source);

    if (!source.data) {
        spdlog::error("Could not open or find the image");
        return false;
    } else {
        Mat undistorted;
        undistort(source, undistorted, m_config->cameraMatrix, m_config->distCoeffs);

        Mat final;
        if (m_config->swapRgb) {
            cvtColor(undistorted, final, COLOR_RGB2BGR);
        } else {
            final = undistorted;
        }

        if (m_config->debug) {
            imwrite(m_config->outputPrefix + "source-" + name + ".jpg", final);
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
    const int wait = 2;
    int i = 1;
    string action;

    while (true) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action != ACTION_IDLE) {
            break;
        }

        if (takePhoto("idle-" + to_string(i))) {
            spdlog::info("Photo !");
        }

        i++;

        this_thread::sleep_for(chrono::seconds(wait));
    }
}

/**
 * Procède à l'étalonnage
 */
void ProcessThread::processEtalonnage() {
    Etalonnage etalonnage(m_config);

    if (takePhoto("etalonnage")) {
        json r = etalonnage.run(m_imgOrig);

        pthread_mutex_lock(&m_datasMutex);
        m_etalonnageResult = r;
        pthread_mutex_unlock(&m_datasMutex);
    }
}

/**
 * Boucle de détection
 */
void ProcessThread::processDetection() {
    Detection detection(m_config);

    const int wait = 2;
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

        this_thread::sleep_for(chrono::seconds(wait));
    }
}
