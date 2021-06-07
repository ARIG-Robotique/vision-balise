#ifndef VISION_BALISE_SCREEN_H
#define VISION_BALISE_SCREEN_H

#include "OledI2C.h"
#include "common.h"
#include "Config.h"

class Screen {

private:
    Config* config;
#ifdef PI
    SSD1306::OledI2C* display;
#else
    SSD1306::OledBitmap<128, 64>* display;
#endif

public:
    explicit Screen(Config* config);

    void showLogo();

    void showInfo(const char* line1, const char* line2);

private:
    void printLn(const char* str, short line);
    void update();
};

#endif //VISION_BALISE_SCREEN_H
