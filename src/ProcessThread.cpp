#include <thread>
#include "ProcessThread.h"
#include "Detection.h"
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
    pthread_cond_signal(&m_actionCond);
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
    bool noWait = true;
    string action;

    while (!stop) {
        pthread_mutex_lock(&m_actionMutex);
        if (noWait) {
            noWait = false;
        } else {
            pthread_cond_wait(&m_actionCond, &m_actionMutex);
        }
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action == ACTION_EXIT) {
            m_video->release();
            spdlog::info("ProcessThread: Demande d'arret du thread");
            stop = true;

        } else if (action == ACTION_IDLE) {
            spdlog::info("ProcessThread: Démarrage de la prise de photo");
            processIdle();

            noWait = true;

        } else if (action == ACTION_DETECTION) {
            spdlog::info("ProcessThread: Démarrage de la détection");
            processDetection();

            noWait = true;

        } else {
            spdlog::warn("ProcessThread: action non supportée");
        }
    }

    pthread_exit(nullptr);
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

        Mat source;
        m_video->read(source);

        if (!source.data) {
            spdlog::error("Could not open or find the image");
        } else {
            spdlog::info("Photo !");

            pthread_mutex_lock(&m_datasMutex);
            if (m_config->swapRgb) {
                m_imgOrig = Mat();
                cvtColor(source, m_imgOrig, COLOR_RGB2BGR);
            } else {
                m_imgOrig = source;
            }
            pthread_mutex_unlock(&m_datasMutex);

            if (m_config->debug) {
                imwrite(m_config->outputPrefix + "source-idle-" + to_string(i) + ".jpg", m_imgOrig);
            }
        }

        i++;

        this_thread::sleep_for(chrono::seconds(wait));
    }
}

/**
 * Boucle de détection
 */
void ProcessThread::processDetection() {
    Detection detection(m_config);

    const int wait = 2;
    string action;
    int i = 0;

    while (true) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action != ACTION_DETECTION) {
            break;
        }

        Mat source;
        m_video->read(source);

        if (!source.data) {
            spdlog::error("Could not open or find the image");
        } else {
            pthread_mutex_lock(&m_datasMutex);
            if (m_config->swapRgb) {
                m_imgOrig = Mat();
                cvtColor(source, m_imgOrig, COLOR_RGB2BGR);
            } else {
                m_imgOrig = source;
            }
            pthread_mutex_unlock(&m_datasMutex);

            if (m_config->debug) {
                imwrite(m_config->outputPrefix + "source-detecion-" + to_string(i) + ".jpg", m_imgOrig);
            }

            json r = detection.run(m_imgOrig, i);

            pthread_mutex_lock(&m_datasMutex);
            m_detectionResult = r;
            pthread_mutex_unlock(&m_datasMutex);
        }

        i++;

        this_thread::sleep_for(chrono::seconds(wait));
    }
}
