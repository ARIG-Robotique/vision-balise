#ifndef VISION_BALISE_PROCESSTHREAD_H
#define VISION_BALISE_PROCESSTHREAD_H

#include "common.h"
#include "Config.h"

class ProcessThread {

private:
    Config* m_config;

    pthread_t m_thread;

    bool m_ready = false;
    bool m_etallonageOk = false;
    json m_detectionResult;
    Mat m_imgOrig;

    pthread_mutex_t m_datasMutex = PTHREAD_MUTEX_INITIALIZER;

    string m_action;
    pthread_cond_t m_actionCond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t m_actionMutex = PTHREAD_MUTEX_INITIALIZER;

public:
    explicit ProcessThread(Config* config);

    bool isReady();
    JsonResult getStatus();
    JsonResult getPhoto(int width);

    JsonResult startEtallonage();
    JsonResult startDetection();

    JsonResult exit();

private:
    void * process();
    static void * create(void* context);

    JsonResult action(const char *_action);

    void processEtallonage();
    void processDetection();
};


#endif //VISION_BALISE_PROCESSTHREAD_H
