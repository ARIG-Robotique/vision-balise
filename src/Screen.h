#ifndef VISION_BALISE_SCREEN_H
#define VISION_BALISE_SCREEN_H

#include "OledI2C.h"
#include "common.h"
#include "Config.h"

class Screen {

private:
    Config* config;
    SSD1306::OledI2C* display;

public:
    explicit Screen(Config* config);

};

#endif //VISION_BALISE_SCREEN_H
