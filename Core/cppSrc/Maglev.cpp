#include "config.hpp"
#include "magnet.hpp"
#include "sensor.hpp"

Sensor::TMAG5273 sensor;

/* 四路电磁铁驱动，定时器 / 通道分配见 config.hpp。 */
Magnet::DRV8870 magnet1(&MAGNET1_TIM_HANDLE, MAGNET1_CH_IN1, MAGNET1_CH_IN2);
Magnet::DRV8870 magnet2(&MAGNET2_TIM_HANDLE, MAGNET2_CH_IN1, MAGNET2_CH_IN2);
Magnet::DRV8870 magnet3(&MAGNET3_TIM_HANDLE, MAGNET3_CH_IN1, MAGNET3_CH_IN2);
Magnet::DRV8870 magnet4(&MAGNET4_TIM_HANDLE, MAGNET4_CH_IN1, MAGNET4_CH_IN2);

void init() {
    HAL_Delay(900); // 等待电压稳定

    /* 启动四路 H 桥的 PWM，此时输出为滑行（不通电）。 */
    magnet1.init();
    magnet2.init();
    magnet3.init();
    magnet4.init();

    /* 默认正转，用于确认四块电磁铁的实际极性：duty 为正表示 IN1 = 1、IN2 = 0，
     * 电流从 OUT1 流向 OUT2。俯视/仰视看磁极方向记下来后，再由闭环控制器接管。 */
    magnet1.setDuty(+MAGNET_DEFAULT_DUTY);
    magnet2.setDuty(+MAGNET_DEFAULT_DUTY);
    magnet3.setDuty(+MAGNET_DEFAULT_DUTY);
    magnet4.setDuty(+MAGNET_DEFAULT_DUTY);
}

void loop() {
    while(true){
        HAL_Delay(1);
    }
}

extern "C" {
    void startup() {
        init();
        loop();
    }

    void TIM2_IRQHandler(){
        
    }
}
