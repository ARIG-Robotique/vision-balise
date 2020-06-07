#ifndef VISION_BALISE_VIDEOTHREAD_H
#define VISION_BALISE_VIDEOTHREAD_H

#include "common.h"
#include "Config.h"

class VideoThread {

private:
    Config* m_config;
    VideoCapture* m_video;

    Mat src;

    bool stop = false;
    bool m_ready = false;
    bool m_cameraReady = false;

    pthread_t m_thread;
    pthread_mutex_t m_datasMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  m_readySignal = PTHREAD_COND_INITIALIZER;

public:
    explicit VideoThread(Config *config) {
        m_config = config;
        m_ready = pthread_create(&m_thread, nullptr, &VideoThread::create, this) != -1;
    };

    void exit() {
        stop = true;
    }

    bool waitReady() {
        if (!m_ready) {
            return false;
        }

        pthread_mutex_lock(&m_datasMutex);
        pthread_cond_wait(&m_readySignal, &m_datasMutex);
        pthread_mutex_unlock(&m_datasMutex);

        return m_cameraReady;
    }

    Mat getPhoto() {
        if (m_video == nullptr) {
            return Mat();
        }

        return src;
    }

private:
    static void * create(void* context) {
        return ((VideoThread *) context)->process();
    }

    void * process() {
        m_video = new VideoCapture(m_config->cameraIndex);

        pthread_mutex_lock(&m_datasMutex);
        m_cameraReady = m_video->isOpened();
        pthread_cond_signal(&m_readySignal);
        pthread_mutex_unlock(&m_datasMutex);

        if (!m_cameraReady) {
            spdlog::error("Cannot open camera");
            pthread_exit(nullptr);
        }

        spdlog::info("VideoThread: ready");

        m_video->set(CV_CAP_PROP_FRAME_WIDTH, m_config->cameraResolution.width);
        m_video->set(CV_CAP_PROP_FRAME_HEIGHT, m_config->cameraResolution.height);
        m_video->set(CV_CAP_PROP_FPS, 20);
        m_video->set(CV_CAP_PROP_BUFFERSIZE, 3);

        while (!this->stop) {
            m_video->read(src);
        }

        m_video->release();
        pthread_exit(nullptr);
    }

};

#endif //VISION_BALISE_VIDEOTHREAD_H
