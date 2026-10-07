#include "config.hpp"

void init() {
    HAL_Delay(900); // 等待电压稳定

}

void loop() {

}

extern "C" {
    void startup() {
        init();
        loop();
    }
}