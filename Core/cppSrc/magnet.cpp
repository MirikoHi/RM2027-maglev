/**
 * @file    magnet.cpp
 * @brief   DRV8870 电磁铁驱动实现（寄存器级说明见 magnet.hpp）
 */

#include "magnet.hpp"

namespace Magnet
{

    namespace
    {
        /**
         * 定时器输入时钟（Hz）。RM0008 时钟树：APB 分频系数为 1 时定时器时钟
         * 等于 PCLK，否则为 PCLK 的 2 倍；TIM1 / TIM8 挂在 APB2 上。
         */
        uint32_t timerClockHz(const TIM_TypeDef* inst) {
            const bool onApb2 = (inst == TIM1) || (inst == TIM8);
            const uint32_t ppre = onApb2 ? ((RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos)
                                         : ((RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos);
            const uint32_t pclk = onApb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
            return (ppre == 0u) ? pclk : (pclk * 2u);
        }
    }  // namespace

    /* ========================================================================== */
    /*                                  构造函数                                   */
    /* ========================================================================== */
    DRV8870::DRV8870(TIM_HandleTypeDef* htim, uint32_t chIn1, uint32_t chIn2)
        : htim_(htim), chIn1_(chIn1), chIn2_(chIn2), duty_(0.0f), enabled_(false), ready_(false) {
    }

    /* ========================================================================== */
    /*                                  初始化                                     */
    /* ========================================================================== */
    bool DRV8870::init() {
        ready_ = false;
        if ((htim_ == nullptr) || (htim_->Instance == nullptr)) {
            return false;
        }

        /* 载波的 PSC / ARR 由 CubeMX 配置，这里只启动通道。 */
        if ((HAL_TIM_PWM_Start(htim_, chIn1_) != HAL_OK) ||
            (HAL_TIM_PWM_Start(htim_, chIn2_) != HAL_OK)) {
            return false;
        }

        duty_ = 0.0f;
        enabled_ = false;
        ready_ = true;
        return apply();
    }

    /* ========================================================================== */
    /*                                输出控制                                     */
    /* ========================================================================== */
    bool DRV8870::apply() {
        if (!ready_) {
            return false;
        }

        if (!enabled_) {
            /* 滑行：两个输入都低，H 桥高阻（1 ms 后器件进入睡眠）。 */
            __HAL_TIM_SET_COMPARE(htim_, chIn1_, 0u);
            __HAL_TIM_SET_COMPARE(htim_, chIn2_, 0u);
            return true;
        }

        const uint32_t period = __HAL_TIM_GET_AUTORELOAD(htim_) + 1u;  // 一个周期的计数次数
        const float magnitude = (duty_ < 0.0f) ? -duty_ : duty_;

        /* PWM1 模式下 CNT < CCR 时输出为高，因此高电平占 (1 - |duty|) 需要
         * CCR = (1 - |duty|) × 周期；CCR = 周期即恒高（比较值允许超过 ARR）。 */
        const auto compare =
            static_cast<uint32_t>(static_cast<float>(period) * (1.0f - magnitude) + 0.5f);

        const bool forward = (duty_ >= 0.0f);
        __HAL_TIM_SET_COMPARE(htim_, forward ? chIn1_ : chIn2_, period);   // 常高：定方向
        __HAL_TIM_SET_COMPARE(htim_, forward ? chIn2_ : chIn1_, compare);  // 调制：调电流
        return true;
    }

    bool DRV8870::setDuty(float duty) {
        if (duty > 1.0f) {
            duty = 1.0f;
        } else if (duty < -1.0f) {
            duty = -1.0f;
        }
        duty_ = duty;
        enabled_ = true;
        return apply();
    }

    bool DRV8870::coast() {
        enabled_ = false;
        return apply();
    }

    uint32_t DRV8870::pwmFrequencyHz() const {
        if (!ready_) {
            return 0u;
        }
        const uint32_t pscPlus1 = htim_->Instance->PSC + 1u;
        const uint32_t arrPlus1 = __HAL_TIM_GET_AUTORELOAD(htim_) + 1u;
        return timerClockHz(htim_->Instance) / pscPlus1 / arrPlus1;
    }

    uint32_t DRV8870::dutySteps() const {
        if (!ready_) {
            return 0u;
        }
        return __HAL_TIM_GET_AUTORELOAD(htim_) + 1u;
    }

}  // namespace Magnet
