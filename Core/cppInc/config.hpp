#pragma once
/**
 * @file    config.hpp
 * @brief   工程用户配置：所有可调参数集中在此。
 */

#include "main.h"

 /* ========================================================================== */
 /*                           TMAG5273 三轴磁传感器                              */
 /* ========================================================================== */

 /** I2C 外设句柄 */
#define TMAG5273_I2C_HANDLE hi2c1
/** 器件 7 位从机地址：A 版 0x35、B 版 0x22、C 版 0x78、D 版 0x44 */
#define TMAG5273_I2C_ADDR 0x35
/** 单次 I2C 传输超时（ms） */
#define TMAG5273_I2C_TIMEOUT_MS 10
/** 从睡眠模式唤醒后的等待时间（ms），Tstart_sleep 典型值 50 µs */
#define TMAG5273_WAKEUP_DELAY_MS 1
/** 触发模式下等待转换完成时附加的裕量（µs） */
#define TMAG5273_CONV_MARGIN_US 100

/** OPERATING_MODE[1:0]：Standby 待机触发 / Sleep 睡眠 / Continuous 连续 / WakeSleep 唤醒-睡眠 */
#define TMAG5273_OP_MODE Sensor::OpMode::Continuous
/** MAG_CH_EN[7:4]：使能的磁通道：Off / X / Y / XY / Z / ZX / YZ / XYZ / XYX / YXY / YZY / XZX */
#define TMAG5273_MAG_CH_EN Sensor::MagChEn::XYZ
/** CONV_AVG[4:2]：平均次数 X1 / X2 / X4 / X8 / X16 / X32，次数越高噪声越低、更新率越低（表 6-4） */
#define TMAG5273_CONV_AVG Sensor::ConvAvg::X1
/** T_CH_EN：温度通道使能 */
#define TMAG5273_TEMP_EN true
/** X_Y_RANGE[1]：Low = A1 ±40 mT / A2 ±133 mT，High = A1 ±80 mT / A2 ±266 mT（表 6-3） */
#define TMAG5273_XY_RANGE Sensor::Range::Low
/** Z_RANGE[0]：同上 */
#define TMAG5273_Z_RANGE Sensor::Range::Low
/** ANGLE_EN[3:2]：器件内部角度计算：Off / XY / YZ / XZ */
#define TMAG5273_ANGLE_EN Sensor::AngleEn::Off
/** SLEEPTIME[3:0]：唤醒-睡眠模式唤醒间隔，Ms1 ~ Ms20000 */
#define TMAG5273_SLEEP_TIME Sensor::SleepTime::Ms1
/** LP_LN：true 低噪声模式，false 低功耗模式 */
#define TMAG5273_LOW_NOISE false
/** MAG_TEMPCO[6:5]：磁体温度补偿：0 不补偿 / 1 NdFeB 0.12 %/℃ / 3 陶瓷 0.2 %/℃ */
#define TMAG5273_MAG_TEMPCO 0
