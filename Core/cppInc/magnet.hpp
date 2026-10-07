#pragma once
/**
 * @file    magnet.hpp
 * @brief   DRV8870 电磁铁驱动：4 路 H 桥，每路驱动一块电磁铁。
 *
 * 每个 DRV8870 由两个 PWM 输入 IN1 / IN2 控制，真值表见手册表 1：
 *
 *   IN1  IN2   OUT1   OUT2   说明
 *    0    0    高阻    高阻   滑行；持续 1 ms 后器件进入睡眠（7.3.2 节）
 *    0    1     L      H     反转，电流 OUT2 → OUT1
 *    1    0     H      L     正转，电流 OUT1 → OUT2
 *    1    1     L      L     刹车，两个低边管导通，电流慢速衰减
 *
 * 本驱动采用 7.3.1 节推荐的「驱动 ↔ 刹车」调制：常高的一路决定电流方向，
 * 另一路高电平占 (1 - |duty|)（高电平期间为刹车续流）。这样 H 桥始终处于
 * 两个有效状态，电流纹波远小于「驱动 ↔ 滑行」，且两个输入不会同时为低，
 * 器件也不会误入睡眠模式。
 *
 * 用法（4 路实例的定时器 / 通道分配见 config.hpp）：
 * @code
 *   Magnet::DRV8870 mag(&MAGNET1_TIM_HANDLE, MAGNET1_CH_IN1, MAGNET1_CH_IN2);
 *   mag.init();        // 启动 PWM，输出置为滑行
 *   mag.setDuty(+0.4f); // 正转，输出幅值 40%
 *   mag.setDuty(-0.4f); // 反转
 * @endcode
 */

#include "config.hpp"
#include "tim.h"

namespace Magnet
{

    /* ========================================================================== */
    /*                                  枚举                                       */
    /* ========================================================================== */
    /** 电流方向，对应表 1 中 OUT1 / OUT2 的电流流向。 */
    enum class Dir : uint8_t
    {
        Forward = 0,  //!< IN1 = 1、IN2 = 0，电流 OUT1 → OUT2
        Reverse = 1,  //!< IN1 = 0、IN2 = 1，电流 OUT2 → OUT1
    };

    /* ========================================================================== */
    /*                                驱动通道                                     */
    /* ========================================================================== */
    /**
     * @brief 单个 DRV8870 通道：一块电磁铁 + 一对 PWM 输入（IN1 / IN2）。
     *
     * 两个 PWM 通道必须来自同一定时器（共用 ARR，载波同步），即手册外围电路
     * 中的一对相邻通道：TIM1_CH1+CH2 / CH3+CH4、TIM8_CH1+CH2 / CH3+CH4。
     * 载波由 CubeMX 配置的 PSC / ARR 决定，本驱动只在运行期读取它们，
     * 不修改定时器配置；占空比的分辨率即一个周期的计数步数（见 dutySteps()）。
     */
    class DRV8870
    {
    public:
        /**
         * @brief 构造一路电磁铁驱动。
         * @param htim  定时器句柄，如 &MAGNET1_TIM_HANDLE
         * @param chIn1 IN1 所在的 PWM 通道，如 MAGNET1_CH_IN1
         * @param chIn2 IN2 所在的 PWM 通道，如 MAGNET1_CH_IN2
         */
        DRV8870(TIM_HandleTypeDef* htim, uint32_t chIn1, uint32_t chIn2);

        /**
         * @brief 启动两路 PWM 并把输出置为滑行（两路都低）。
         * @return true 表示初始化成功
         */
        bool init();

        /** @return 是否已成功初始化。 */
        bool isReady() const
        {
            return ready_;
        }

        /**
         * @brief 设置归一化输出，符号表示电流方向。
         * @param duty 取值范围 [-1, +1]，正为正转、负为反转，超出部分自动限幅；
         *             0 表示刹车（两个低边管导通）
         * @return true 表示已写入比较寄存器
         */
        bool setDuty(float duty);

        /** @return 最近一次设定的 duty（含符号）。 */
        float duty() const
        {
            return duty_;
        }

        /** @return 最近一次设定的电流方向。 */
        Dir direction() const
        {
            return (duty_ < 0.0f) ? Dir::Reverse : Dir::Forward;
        }

        /** @brief 刹车：两路都拉高，低边续流，电流慢速衰减。 */
        bool brake()
        {
            return setDuty(0.0f);
        }

        /** @brief 关断输出：两路都拉低，H 桥高阻，1 ms 后器件进入睡眠。 */
        bool coast();

        /** @return 当前载波频率（Hz）；未初始化时返回 0。 */
        uint32_t pwmFrequencyHz() const;

        /**
         * @return 一个 PWM 周期的计数步数（ARR + 1），即占空比的量化档数。
         *         例：CubeMX 的 Prescaler = 72-1、Period = 20-1 时 TIM1 为 1 MHz /
         *         20 = 50 kHz，但只有 20 档，占空比最小步进 5 %，对电流环偏粗。
         */
        uint32_t dutySteps() const;

    private:
        /** 按 duty_ 刷新两个比较寄存器。 */
        bool apply();

        TIM_HandleTypeDef* htim_;  //!< 定时器句柄（两路通道共用）
        uint32_t chIn1_;           //!< IN1 通道
        uint32_t chIn2_;           //!< IN2 通道
        float duty_;               //!< 归一化输出，[-1, +1]
        bool enabled_;             //!< false 时两路都拉低（滑行）
        bool ready_;               //!< init() 是否成功
    };

}  // namespace Magnet
