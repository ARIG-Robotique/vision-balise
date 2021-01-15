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

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;

        pthread_mutex_lock(&m_datasMutex);
        pthread_cond_timedwait(&m_readySignal, &m_datasMutex, &ts);
        pthread_mutex_unlock(&m_datasMutex);

        if (!m_cameraReady) {
            spdlog::error("Cannot access camera within 5 seconds)");
        }

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

        if (!m_video->isOpened()) {
            spdlog::error("Cannot open camera");
            pthread_cond_signal(&m_readySignal);
            pthread_mutex_unlock(&m_datasMutex);
            pthread_exit(nullptr);
        }

        spdlog::info("VideoThread: ready");

        m_video->set(CV_CAP_PROP_FRAME_WIDTH, m_config->cameraResolution.width);
        m_video->set(CV_CAP_PROP_FRAME_HEIGHT, m_config->cameraResolution.height);
        m_video->set(CV_CAP_PROP_FPS, 20);
        m_video->set(CV_CAP_PROP_BUFFERSIZE, 3);

        bool first = true;
        while (!this->stop) {
            m_video->read(src);

            if (first && src.data) {
                m_cameraReady = true;
                pthread_cond_signal(&m_readySignal);
                pthread_mutex_unlock(&m_datasMutex);
                first = false;
            }
        }

        m_video->release();
        pthread_exit(nullptr);
    }

};

#endif //VISION_BALISE_VIDEOTHREAD_H
