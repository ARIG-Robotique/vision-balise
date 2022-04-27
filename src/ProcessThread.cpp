#include <thread>
#include "ProcessThread.h"
#include "Detection.h"
#include "Etalonnage.h"
#include "utils.h"

ProcessThread::ProcessThread(Config *config) {
    m_config = config;

    m_screen = new Screen(config);

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
    json data;

    pthread_mutex_lock(&m_dataMutex);
    data["etalonnageDone"] = m_config->etalonnageDone;
    data["detection"] = m_detectionResult;
    pthread_mutex_unlock(&m_dataMutex);

    JsonResult r;
    r.status = RESPONSE_OK;
    r.data = data;
    return r;
}

/**
 * Retourne la dernière photo en base64
 * @return
 */
JsonResult ProcessThread::getPhoto() {
    json data;

    pthread_mutex_lock(&m_dataMutex);
    if (!m_imgOrig.empty()) {
        data = arig_utils::matToBase64(m_imgOrig);
    }
    pthread_mutex_unlock(&m_dataMutex);

    JsonResult r;

    if (data.empty()) {
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Pas d'image";
    } else {
        r.status = RESPONSE_OK;
        r.data = data;
    }

    return r;
}

JsonResult ProcessThread::setIdle() {
    JsonResult r = action(ACTION_IDLE);
    m_config->reset();
    return r;
}

/**
 * POUR TESTS UNIQUEMENT pas thread safe
 */
Mat &ProcessThread::getImgOrig() {
    return m_imgOrig;
}

/**
 * Démarre la détection dans le sous process
 * @return
 */
JsonResult ProcessThread::startDetection() {
    if (!m_config->etalonnageDone) {
        JsonResult r;
        r.status = RESPONSE_ERROR;
        r.errorMessage = "L'étallonage n'est pas fait";
        return r;
    }

    return action(ACTION_DETECTION);
}

/**
 * Démarre l'étalonnage dans le sous process
 * @return
 */
JsonResult ProcessThread::startEtalonnage() {
    action(ACTION_IDLE);

    Etalonnage etalonnage(m_config);

    if (takePhoto()) {
        return etalonnage.run(m_imgOrig);
    } else {
        JsonResult r;
        r.status = RESPONSE_ERROR;
        r.errorMessage = "Impossible de prendre une photo";
        return r;
    }
}

/**
 * Demande au sous process de s'arreter
 * @return
 */
void ProcessThread::exit() {
    action(ACTION_EXIT);
    updateScreen();
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

        updateScreen();

        if (action.empty()) {
            spdlog::debug("ProcessThread: no action");
            this_thread::sleep_for(chrono::milliseconds(m_config->idleDelay));

        } else if (action == ACTION_EXIT) {
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
            this_thread::sleep_for(chrono::milliseconds(m_config->idleDelay));
        }
    }

    pthread_exit(nullptr);
}

bool ProcessThread::takePhoto() {
    auto start = arig_utils::startTiming();

    Mat source;
    if (!m_config->mockPhoto.empty()) {
        source = imread(m_config->mockPhoto, CV_LOAD_IMAGE_COLOR);

        if (!source.data) {
            spdlog::error("Could not open or find the mock image");
            return false;
        }
    } else {
        source = m_videoThread->getPhoto();

        if (!source.data) {
            spdlog::error("Could not open or find the camera");
            return false;
        }
    }

    Mat undistorted;
    if (m_config->undistort) {
        remap(source, undistorted, m_config->remap1, m_config->remap2, INTER_LINEAR);
    } else {
        undistorted = source;
    }

    Mat final;
    if (m_config->swapRgb) {
        cvtColor(undistorted, final, COLOR_RGB2BGR);
    } else {
        final = undistorted;
    }

    pthread_mutex_lock(&m_dataMutex);
    m_imgOrig = final;
    pthread_mutex_unlock(&m_dataMutex);

    spdlog::info("Photo prise en {}ms", arig_utils::ellapsedTime(start));
    return true;
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

        if (takePhoto()) {
            if (m_config->mockPhoto.empty() && (m_config->debug || i % 10 == 0)) {
                imwrite(m_config->outputPrefix + "idle-" + to_string(i) + ".jpg", m_imgOrig);
            }
        }

        i++;

        updateScreen();
        this_thread::sleep_for(chrono::milliseconds(m_config->idleDelay));
    }
}

/**
 * Boucle de détection
 */
void ProcessThread::processDetection() {
    Detection detection(m_config);

    string action;

    while (true) {
        pthread_mutex_lock(&m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action != ACTION_DETECTION) {
            break;
        }

        if (takePhoto()) {
            json r = detection.run(m_imgOrig);

            pthread_mutex_lock(&m_dataMutex);
            m_detectionResult = r;
            pthread_mutex_unlock(&m_dataMutex);
        }

        updateScreen();
        this_thread::sleep_for(chrono::milliseconds(m_config->detectionDelay));
    }
}

void ProcessThread::updateScreen() {
    if (m_action == ACTION_EXIT) {
        m_screen->clear();

    } else if (m_action == ACTION_IDLE) {
        if (m_config->etalonnageDone) {
            m_screen->showInfo("Etalonnage done", "Team:" + m_config->team);
        } else {
            m_screen->showLogo();
        }

    } else if (m_action == ACTION_DETECTION) {
        if (!m_detectionResult.empty()) {
            m_screen->showDetection(m_detectionResult);
        } else {
            m_screen->showInfo("Détection", "Team:" + m_config->team);
        }

    } else {
        m_screen->showInfo("N/A", "");
    }
}