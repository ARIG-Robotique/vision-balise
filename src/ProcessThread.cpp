#include <thread>
#include "ProcessThread.h"
#include "Etallonage.h"
#include "Detection.h"
#include "utils.h"

ProcessThread::ProcessThread(Config *config) {
    m_config = config;
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
    datas["etallonageOk"] = m_etallonageOk;
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
    }
    else {
        r.status = RESPONSE_OK;
        r.datas = arig_utils::matToBase64(img);
    }

    return r;
}

/**
 * Démarre l'étalonnage dans le sous process
 * @return
 */
JsonResult ProcessThread::startEtallonage() {
    return action(ACTION_ETALLONAGE);
}

/**
 * Démarre la détection dans le sous process
 * @return
 */
JsonResult ProcessThread::startDetection() {
    bool etallonageOk;

    pthread_mutex_lock(&m_datasMutex);
    etallonageOk = m_etallonageOk;
    pthread_mutex_unlock(&m_datasMutex);

    if (!etallonageOk) {
        JsonResult r;
        r.status = RESPONSE_ERROR;
        r.errorMessage = "L'étallonage n'est pas terminé/en échec";
        return r;
    }

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
    bool stop = false;
    string action;

    while (!stop) {
        pthread_mutex_lock(&m_actionMutex);
        pthread_cond_wait(&m_actionCond, &m_actionMutex);
        action = m_action;
        pthread_mutex_unlock(&m_actionMutex);

        if (action == ACTION_EXIT) {
            spdlog::info("ProcessThread: Demande d'arret du thread");
            stop = true;

        } else if (action == ACTION_ETALLONAGE) {
            spdlog::info("ProcessThread: Démarrage de l'étallonage");
            processEtallonage();

        } else if (action == ACTION_DETECTION) {
            spdlog::info("ProcessThread: Démarrage de la détection");
            processDetection();

        } else {
            spdlog::warn("ProcessThread: action non supportée");
        }
    }

    pthread_exit(nullptr);
}

/**
 * Boucle d'étallonage
 */
void ProcessThread::processEtallonage() {
    Etallonage etallonage(m_config);

    const int tries = 4;
    const int wait = 2;
    bool ok = false;
    int i = 0;

    while (!ok && i < tries) {
        // TODO camera
        Mat source = imread("samples/DS1_1526.jpg", IMREAD_COLOR);

        if (!source.data) {
            spdlog::error("Could not open or find the image");
        } else {
            ok = etallonage.run(source, i);
        }

        i++;

        this_thread::sleep_for(chrono::seconds(wait));
    }

    pthread_mutex_lock(&m_datasMutex);
    m_etallonageOk = ok;
    pthread_mutex_unlock(&m_datasMutex);
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

        if (action == ACTION_EXIT) {
            pthread_exit(nullptr);
        }

        // TODO camera
        Mat source = imread("samples/DS1_1526.jpg", IMREAD_COLOR);

        if (!source.data) {
            spdlog::error("Could not open or find the image");
        } else {
            json r = detection.run(source, i);

            pthread_mutex_lock(&m_datasMutex);
            m_detectionResult = r;
            m_imgOrig = source;
            pthread_mutex_unlock(&m_datasMutex);
        }

        i++;

        this_thread::sleep_for(chrono::seconds(wait));
    }
}
