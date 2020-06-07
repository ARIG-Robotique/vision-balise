#ifndef VISION_BALISE_PROCESSTHREAD_H
#define VISION_BALISE_PROCESSTHREAD_H

#include "common.h"
#include "Config.h"
#include "VideoThread.h"

class ProcessThread {

private:
    Config* m_config;
    VideoThread* m_videoThread;

    pthread_t m_thread;
    pthread_mutex_t m_datasMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t m_actionMutex = PTHREAD_MUTEX_INITIALIZER;

    bool m_ready = false;
    json m_detectionResult;
    Mat m_imgOrig;
    string m_action;

public:
    explicit ProcessThread(Config* config);

    bool isReady();
    JsonResult getStatus();
    JsonResult getPhoto();

    JsonResult startDetection();
    JsonResult startEtalonnage();

    JsonResult exit();

    void displayPhoto();

private:
    void * process();
    static void * create(void* context);

    JsonResult action(const char *_action);

    void processIdle();
    void processDetection();

    bool takePhoto(const string &name);
};


#endif //VISION_BALISE_PROCESSTHREAD_H
