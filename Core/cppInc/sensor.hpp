#pragma once
#include "config.hpp"
#include "i2c.h"

namespace Sensor
{

    /* ========================================================================== */
    /*                            出厂 I2C 从机地址                                */
    /* ========================================================================== */
    /** 表 6-2：A/B/C/D 四种版本出厂地址不同，1/2 后缀（量程不同）地址相同。 */
    inline constexpr uint8_t kAddrA = 0x35;  // TMAG5273x1/x2-A
    inline constexpr uint8_t kAddrB = 0x22;  // TMAG5273x1/x2-B
    inline constexpr uint8_t kAddrC = 0x78;  // TMAG5273x1/x2-C
    inline constexpr uint8_t kAddrD = 0x44;  // TMAG5273x1/x2-D

    /* ========================================================================== */
    /*                              寄存器地址（表 8-2）                           */
    /* ========================================================================== */
    enum class Reg : uint8_t
    {
        DeviceConfig1 = 0x00,  //!< CRC 使能、磁体温度补偿、平均次数、I2C 读命令格式
        DeviceConfig2 = 0x01,  //!< 阈值迟滞、低功耗/低噪声、触发模式、工作模式
        SensorConfig1 = 0x02,  //!< 磁通道使能、W&S 模式唤醒间隔
        SensorConfig2 = 0x03,  //!< 角度计算通道、X/Y 量程、Z 量程
        XThrConfig = 0x04,  //!< X 轴磁阈值
        YThrConfig = 0x05,  //!< Y 轴磁阈值
        ZThrConfig = 0x06,  //!< Z 轴磁阈值
        TConfig = 0x07,  //!< 温度阈值、温度通道使能
        IntConfig1 = 0x08,  //!< 中断配置
        MagGainConfig = 0x09,  //!< 增益校正值
        MagOffsetConfig1 = 0x0A,  //!< 第一通道偏置校正
        MagOffsetConfig2 = 0x0B,  //!< 第二通道偏置校正
        I2CAddress = 0x0C,  //!< I2C 地址寄存器（复位值 6Ah）
        DeviceId = 0x0D,  //!< 器件版本 VER[1:0]
        ManufacturerIdLsb = 0x0E,  //!< 厂商 ID 低字节（49h = 'I'）
        ManufacturerIdMsb = 0x0F,  //!< 厂商 ID 高字节（54h = 'T'）
        TMsbResult = 0x10,  //!< 温度结果 [15:8]
        TLsbResult = 0x11,  //!< 温度结果 [7:0]
        XMsbResult = 0x12,  //!< X 轴结果 [15:8]
        XLsbResult = 0x13,  //!< X 轴结果 [7:0]
        YMsbResult = 0x14,  //!< Y 轴结果 [15:8]
        YLsbResult = 0x15,  //!< Y 轴结果 [7:0]
        ZMsbResult = 0x16,  //!< Z 轴结果 [15:8]
        ZLsbResult = 0x17,  //!< Z 轴结果 [7:0]
        ConvStatus = 0x18,  //!< 转换状态
        AngleResultMsb = 0x19,  //!< 角度结果 [15:8]
        AngleResultLsb = 0x1A,  //!< 角度结果 [7:0]
        MagnitudeResult = 0x1B,  //!< 合成矢量幅值结果
        DeviceStatus = 0x1C,  //!< 器件状态 / 错误标志
    };

    /** 一次读取全部结果寄存器所需长度：0x10（T_MSB）~ 0x1B（MAGNITUDE_RESULT）。 */
    inline constexpr uint16_t kResultBlockLen = 12;

    /* ========================================================================== */
    /*                                  位定义                                     */
    /* ========================================================================== */
    namespace Bits
    {
        // DEVICE_CONFIG_1 (0x00)
        inline constexpr uint8_t kCrcEn = 7;  // 1 位
        inline constexpr uint8_t kMagTempco = 5;  // [6:5]
        inline constexpr uint8_t kConvAvg = 2;  // [4:2]
        inline constexpr uint8_t kI2CRd = 0;  // [1:0]
        // DEVICE_CONFIG_2 (0x01)
        inline constexpr uint8_t kThrHyst = 5;  // [7:5]
        inline constexpr uint8_t kLpLn = 4;  // 1 位：0 = 低功耗，1 = 低噪声
        inline constexpr uint8_t kI2CGlitchFilt = 3;  // 1 位：0 = 滤波开，1 = 滤波关
        inline constexpr uint8_t kTriggerMode = 2;  // 1 位：0 = I2C 触发，1 = INT 引脚触发
        inline constexpr uint8_t kOperatingMode = 0;  // [1:0]
        // SENSOR_CONFIG_1 (0x02)
        inline constexpr uint8_t kMagChEn = 4;  // [7:4]
        inline constexpr uint8_t kSleepTime = 0;  // [3:0]
        // SENSOR_CONFIG_2 (0x03)
        inline constexpr uint8_t kAngleEn = 2;  // [3:2]
        inline constexpr uint8_t kXYRange = 1;  // 1 位
        inline constexpr uint8_t kZRange = 0;  // 1 位
        // T_CONFIG (0x07)
        inline constexpr uint8_t kTChEn = 0;  // 1 位
        // INT_CONFIG_1 (0x08)
        inline constexpr uint8_t kRsltInt = 7;  // 1 位：转换完成中断使能
        inline constexpr uint8_t kThrsldInt = 6;  // 1 位：阈值越界中断使能
        inline constexpr uint8_t kIntState = 5;  // 1 位：0 = 锁存，1 = 10us 脉冲
        inline constexpr uint8_t kIntMode = 2;  // [4:2]
        inline constexpr uint8_t kMaskIntb = 0;  // 1 位：INT 接地时置 1
        // DEVICE_ID (0x0D)
        inline constexpr uint8_t kVersion = 0;  // [1:0] 器件型号版本
        // CONV_STATUS (0x18)
        inline constexpr uint8_t kSetCount = 5;  // [7:5] 滚动计数
        inline constexpr uint8_t kPor = 4;  // R/W1CP：上电复位标志
        inline constexpr uint8_t kDiagStatus = 1;  // 内部诊断故障
        inline constexpr uint8_t kResultStatus = 0;  // 转换数据就绪
        // DEVICE_STATUS (0x1C)
        inline constexpr uint8_t kIntbRb = 4;  // INT 引脚回读电平
        inline constexpr uint8_t kOscEr = 3;  // R/W1CP：振荡器故障
        inline constexpr uint8_t kIntEr = 2;  // R/W1CP：INT 引脚故障
        inline constexpr uint8_t kOtpCrcEr = 1;  // R/W1CP：OTP CRC 故障
        inline constexpr uint8_t kVccUvEr = 0;  // R/W1CP：VCC 欠压故障
        // 读/写事务中「寄存器地址字节」的最高位 = 转换触发位（6.5.1.3.1）
        inline constexpr uint8_t kTriggerBit = 7;
    }  // namespace Bits

    /* ========================================================================== */
    /*                                寄存器枚举量                                 */
    /* ========================================================================== */
    /** DEVICE_CONFIG_1[4:2]：平均次数，同时决定更新速率（表 6-4）。 */
    enum class ConvAvg : uint8_t
    {
        X1 = 0,  //!< 1 次平均，10.0 kSPS（三轴）
        X2 = 1,  //!< 2 次平均， 5.7 kSPS
        X4 = 2,  //!< 4 次平均， 3.1 kSPS
        X8 = 3,  //!< 8 次平均， 1.6 kSPS
        X16 = 4,  //!< 16 次平均，0.8 kSPS
        X32 = 5,  //!< 32 次平均，0.4 kSPS
    };

    /** DEVICE_CONFIG_2[1:0]：工作模式。 */
    enum class OpMode : uint8_t
    {
        Standby = 0,  //!< 待机（触发）模式：由 I2C 命令或 INT 引脚触发单次转换
        Sleep = 1,  //!< 睡眠模式：不可访问寄存器，I2C 地址即可唤醒
        Continuous = 2,  //!< 连续测量模式：按配置自由连续转换
        WakeSleep = 3,  //!< 唤醒-睡眠模式：按 SLEEPTIME 间隔周期性测量
    };

    /** SENSOR_CONFIG_1[7:4]：使能的磁通道。 */
    enum class MagChEn : uint8_t
    {
        Off = 0,  //!< 全部关闭
        X = 1,  //!< 仅 X
        Y = 2,  //!< 仅 Y
        XY = 3,  //!< X + Y
        Z = 4,  //!< 仅 Z
        ZX = 5,  //!< Z + X
        YZ = 6,  //!< Y + Z
        XYZ = 7,  //!< X + Y + Z
        XYX = 8,  //!< 准同时采样：X-Y-X
        YXY = 9,  //!< 准同时采样：Y-X-Y
        YZY = 10, //!< 准同时采样：Y-Z-Y
        XZX = 11, //!< 准同时采样：X-Z-X
    };

    /** SENSOR_CONFIG_2[3:2]：角度计算所用的轴对。 */
    enum class AngleEn : uint8_t
    {
        Off = 0,  //!< 不做角度计算
        XY = 1,  //!< 第一通道 X，第二通道 Y
        YZ = 2,  //!< 第一通道 Y，第二通道 Z
        XZ = 3,  //!< 第一通道 X，第二通道 Z
    };

    /** SENSOR_CONFIG_2[1:0]：量程档位，实际满量程取决于器件型号（表 6-3）。 */
    enum class Range : uint8_t
    {
        Low = 0,  //!< A1: ±40 mT，A2: ±133 mT
        High = 1,  //!< A1: ±80 mT，A2: ±266 mT
    };

    /** SENSOR_CONFIG_1[3:0]：唤醒-睡眠模式下的唤醒间隔。 */
    enum class SleepTime : uint8_t
    {
        Ms1 = 0x0,  //!< 1 ms
        Ms5 = 0x1,  //!< 5 ms
        Ms10 = 0x2,  //!< 10 ms
        Ms15 = 0x3,  //!< 15 ms
        Ms20 = 0x4,  //!< 20 ms
        Ms30 = 0x5,  //!< 30 ms
        Ms50 = 0x6,  //!< 50 ms
        Ms100 = 0x7,  //!< 100 ms
        Ms500 = 0x8,  //!< 500 ms
        Ms1000 = 0x9,  //!< 1000 ms
        Ms2000 = 0xA,  //!< 2000 ms
        Ms5000 = 0xB,  //!< 5000 ms
        Ms20000 = 0xC,  //!< 20000 ms
    };

    /* ========================================================================== */
    /*                                  换算常量                                   */
    /* ========================================================================== */
    /** 6.5.2.1 节式(10)：B[mT] = 原始码 / 2^15 × 满量程[mT]，原始码为 16 位补码。 */
    inline constexpr float kMagLsbScale = 1.0f / 32768.0f;
    /** 5.6 节：TSENS_T0 = 25 ℃ 对应 16 位码 17508，温度分辨率 58 LSB/℃。 */
    inline constexpr float   kTempRefCelsius = 25.0f;
    inline constexpr int16_t kTempRefRaw = 17508;
    inline constexpr float   kTempLsbPerDegC = 58.0f;
    /** 6.5.2.3 节式(14)：角度整数部分 9 位，小数部分 4 位，格式 (xxxx/16)。 */
    inline constexpr float kAngleFracLsb = 1.0f / 16.0f;
    /** 厂商 ID：MSB = 54h('T')，LSB = 49h('I')。 */
    inline constexpr uint16_t kManufacturerId = 0x5449;
    /** HAL I2C 单次传输超时（ms），由 config.hpp 配置。 */
    inline constexpr uint32_t kI2CTimeoutMs = TMAG5273_I2C_TIMEOUT_MS;

    /* ========================================================================== */
    /*                                  工具函数                                   */
    /* ========================================================================== */
    /** 取出 value 的 [pos + width - 1 : pos] 位域。 */
    [[nodiscard]] constexpr uint8_t getField(uint8_t value, uint8_t pos, uint8_t width) {
        return static_cast<uint8_t>((value >> pos) & ((1u << width) - 1u));
    }

    /** 将 field 写入 value 的 [pos + width - 1 : pos] 位域，其余位保持不变。 */
    [[nodiscard]] constexpr uint8_t setField(uint8_t value, uint8_t pos, uint8_t width, uint8_t field) {
        const uint8_t mask = static_cast<uint8_t>(((1u << width) - 1u) << pos);
        return static_cast<uint8_t>((value & static_cast<uint8_t>(~mask)) |
            (static_cast<uint8_t>(field << pos) & mask));
    }

    /** 16 位磁场原始码 → 毫特斯拉。 */
    [[nodiscard]] inline float rawToMilliTesla(int16_t raw, float fullScaleMt) {
        return static_cast<float>(raw) * kMagLsbScale * fullScaleMt;
    }

    /** 温度原始码 → 摄氏度（式(12)）。 */
    [[nodiscard]] inline float rawToCelsius(int16_t raw) {
        return kTempRefCelsius + (static_cast<float>(raw) - static_cast<float>(kTempRefRaw)) /
            kTempLsbPerDegC;
    }

    /** 角度原始码 → 度（式(14)：bit15~13 恒为 0，bit12~4 为整数，bit3~0 为小数）。 */
    [[nodiscard]] inline float rawToDegree(uint16_t raw) {
        return static_cast<float>(raw >> 4) + static_cast<float>(raw & 0x000Fu) * kAngleFracLsb;
    }

    /* ========================================================================== */
    /*                                  配置结构                                   */
    /* ========================================================================== */
    /** 器件配置，字段与寄存器一一对应（图 8-2 各寄存器），默认值全部取自 config.hpp。 */
    struct Config
    {
        uint8_t   addr = TMAG5273_I2C_ADDR;         //!< 7 位 I2C 地址（表 6-2）
        ConvAvg   avg = TMAG5273_CONV_AVG;          //!< CONV_AVG：平均次数 / 更新速率
        MagChEn   channels = TMAG5273_MAG_CH_EN;    //!< MAG_CH_EN：使能的磁通道
        bool      tempEn = TMAG5273_TEMP_EN;        //!< T_CH_EN：温度通道使能
        Range     xyRange = TMAG5273_XY_RANGE;      //!< X_Y_RANGE：X/Y 轴量程
        Range     zRange = TMAG5273_Z_RANGE;        //!< Z_RANGE：Z 轴量程
        AngleEn   angleEn = TMAG5273_ANGLE_EN;      //!< ANGLE_EN：角度计算轴对
        OpMode    mode = TMAG5273_OP_MODE;          //!< OPERATING_MODE：工作模式
        SleepTime sleepTime = TMAG5273_SLEEP_TIME;  //!< SLEEPTIME：W&S 模式唤醒间隔
        bool      lowNoise = TMAG5273_LOW_NOISE;    //!< LP_LN：false 低功耗 / true 低噪声
        uint8_t   magTempco = TMAG5273_MAG_TEMPCO;  //!< MAG_TEMPCO[1:0]：0 无补偿、1 NdFeB、3 陶瓷
    };

    /* ========================================================================== */
    /*                                  采样结果                                   */
    /* ========================================================================== */
    /** 一次完整采样：同时给出寄存器原始码与换算后的物理量。 */
    struct Sample
    {
        int16_t tRaw = 0;      //!< 温度原始码（16 位补码）
        int16_t xRaw = 0;      //!< X 轴原始码（16 位补码）
        int16_t yRaw = 0;      //!< Y 轴原始码
        int16_t zRaw = 0;      //!< Z 轴原始码
        uint16_t angleRaw = 0;      //!< 角度原始码（13 位）
        uint8_t magnitude = 0;      //!< 合成矢量幅值原始码（8 位）
        float   t = 0.0f;   //!< 温度，℃
        float   x = 0.0f;   //!< X 轴磁感应强度，mT
        float   y = 0.0f;   //!< Y 轴磁感应强度，mT
        float   z = 0.0f;   //!< Z 轴磁感应强度，mT
        float   angle = 0.0f;   //!< 角度，度（ANGLE_EN = Off 时无效）
        bool    ready = false;  //!< CONV_STATUS.RESULT_STATUS：转换数据就绪
        bool    diagFault = false;  //!< CONV_STATUS.DIAG_STATUS：内部诊断故障
        bool    por = false;  //!< CONV_STATUS.POR：发生过上电复位
    };

    /* ========================================================================== */
    /*                                 驱动主类                                    */
    /* ========================================================================== */
    /**
     * @brief TMAG5273 三轴磁传感器驱动。
     *
     * 典型用法：配置全部写在 config.hpp 里，此处无需再传参数。
     * @code
     *   Sensor::TMAG5273 mag;              // 用 config.hpp 的句柄与地址
     *   if (!mag.init()) { Error_Handler(); }
     *   Sensor::Sample s;
     *   while (true) {
     *       if (mag.readSample(s)) { ... 使用 s.x / s.y / s.z ... }
     *   }
     * @endcode
     *
     * 说明：本驱动使用 I2C_RD = 00b（标准 3 字节读）与 CRC_EN = 0b（复位默认值），
     *       即读事务格式为「START + 地址(W) + 寄存器地址，RESTART + 地址(R) + N 字节数据」，
     *       与 HAL_I2C_Mem_Read() 产生的时序一致（6.5.1.3.3 节图 6-9）。
     */
    class TMAG5273
    {
    public:
        /**
         * @brief 构造驱动对象，两个参数默认取自 config.hpp。
         * @param hi2c  I2C 外设句柄，默认 &TMAG5273_I2C_HANDLE
         * @param addr7 器件 7 位地址，默认 TMAG5273_I2C_ADDR
         */
        explicit TMAG5273(I2C_HandleTypeDef* hi2c = &TMAG5273_I2C_HANDLE,
                          uint8_t addr7 = TMAG5273_I2C_ADDR);

        /* ------------------------------ 初始化 ------------------------------ */
        /**
         * @brief 唤醒器件、校验厂商 ID 与型号，并写入配置。
         * @param cfg 目标配置
         * @return true 表示器件在线且配置全部写入成功
         */
        bool init(const Config& cfg = Config{});

        /** @brief 按当前配置重新写入全部配置寄存器。 */
        bool applyConfig();

        /** @brief 检测器件是否在线（读取厂商 ID）。 */
        [[nodiscard]] bool isConnected();

        /* ------------------------------ 信息读取 ---------------------------- */
        /** @brief 读取厂商 ID，正常应为 0x5449（"TI"）。 */
        bool manufacturerId(uint16_t& id);

        /** @brief 读取 DEVICE_ID.VER[1:0]：1 = ±40/±80 mT 型号，2 = ±133/±266 mT 型号。 */
        bool readVersion(uint8_t& version);

        /** @brief 缓存到本对象的型号版本（init() 后有效）。 */
        [[nodiscard]] uint8_t version() const { return version_; }

        /**
         * @brief 指定轴的满量程（mT）。
         * @param axis 0 = X，1 = Y，2 = Z
         */
        [[nodiscard]] float fullScaleMt(uint8_t axis) const;

        /* ------------------------------ 运行配置 ---------------------------- */
        /** @brief 单独设置工作模式（DEVICE_CONFIG_2[1:0]）。 */
        bool setOperatingMode(OpMode mode);

        /** @brief 单独设置使能的磁通道（SENSOR_CONFIG_1[7:4]）。 */
        bool setChannels(MagChEn channels);

        /** @brief 单独设置 X/Y、Z 轴量程（SENSOR_CONFIG_2[1:0]）。 */
        bool setRanges(Range xy, Range z);

        /** @brief 单独设置平均次数（DEVICE_CONFIG_1[4:2]）。 */
        bool setAveraging(ConvAvg avg);

        /** @brief 单独设置角度计算轴对（SENSOR_CONFIG_2[3:2]）。 */
        bool setAngleChannel(AngleEn angleEn);

        /** @brief 单独使能/关闭温度通道（T_CONFIG[0]）。 */
        bool enableTemperature(bool enable);

        /** @brief 读取当前配置的副本。 */
        [[nodiscard]] const Config& config() const { return cfg_; }

        /** @brief 运行时修改 7 位从机地址（I2C_ADDRESS 寄存器，掉电不保存）。 */
        bool setI2CAddress(uint8_t addr7);

        /* --------------------------- 转换与数据读取 -------------------------- */
        /**
         * @brief 待机（触发）模式下发起一次转换。
         * @note  利用寄存器地址字节的最高位作为触发位（6.5.1.3.1 节），
         *        写入 CONV_STATUS 的 0x00 不会改变任何状态位。
         */
        bool trigger();

        /** @brief 读取 CONV_STATUS.RESULT_STATUS，判断转换数据是否就绪。 */
        bool dataReady(bool& ready);

        /** @brief 轮询等待转换完成，超时时间为 timeoutMs 毫秒。 */
        bool waitDataReady(uint32_t timeoutMs = 5);

        /**
         * @brief 一次性读取 0x10 ~ 0x1B 全部结果寄存器并换算。
         * @param sample 输出采样结果
         */
        bool readSample(Sample& sample);

        /** @brief 只读取 X/Y/Z 原始码（0x12 ~ 0x17，6 字节，最快路径）。 */
        bool readMagRaw(int16_t& x, int16_t& y, int16_t& z);

        /** @brief 只读取 X/Y/Z 并换算为 mT。 */
        bool readMag(float& x, float& y, float& z);

        /** @brief 只读取温度并换算为 ℃。 */
        bool readTemperature(float& celsius);

        /** @brief 只读取角度并换算为度（需先设置 ANGLE_EN）。 */
        bool readAngle(float& degree);

        /** @brief 读取 CONV_STATUS 整字节。 */
        bool readConvStatus(uint8_t& status);

        /** @brief 读取 DEVICE_STATUS 整字节。 */
        bool readDeviceStatus(uint8_t& status);

        /**
         * @brief 清除 DEVICE_STATUS 中的错误标志（R/W1CP：写 1 清除）。
         * @param mask Bits::kOscEr | Bits::kIntEr | Bits::kOtpCrcEr | Bits::kVccUvEr 的组合
         */
        bool clearDeviceErrors(uint8_t mask);

        /* ------------------------------ 底层访问 ---------------------------- */
        /** @brief 从 start 开始连续读取 len 字节寄存器。 */
        bool readRegs(Reg start, uint8_t* data, uint16_t len);

        /** @brief 读取单个寄存器。 */
        bool readReg(Reg reg, uint8_t& value);

        /** @brief 写入单个寄存器。 */
        bool writeReg(Reg reg, uint8_t value);

        /* ------------------------------ 时间估算 ---------------------------- */
        /** @brief 按当前配置估算一次完整转换所需时间（µs），用于触发模式下的等待。 */
        [[nodiscard]] uint32_t conversionTimeUs() const;

        /** @brief 当前使能的磁通道数量。 */
        [[nodiscard]] uint8_t magChannelCount() const;

    private:
        /** 从睡眠模式唤醒：睡眠时从机不响应 ACK，需空事务唤醒后等待 Tstart_sleep。 */
        void wakeUp();

        bool writeConfig1();  //!< 写 DEVICE_CONFIG_1
        bool writeConfig2();  //!< 写 DEVICE_CONFIG_2
        bool writeSensorConfig1();  //!< 写 SENSOR_CONFIG_1
        bool writeSensorConfig2();  //!< 写 SENSOR_CONFIG_2
        bool writeTConfig();  //!< 写 T_CONFIG

        I2C_HandleTypeDef* hi2c_;       //!< I2C 句柄
        uint16_t           addr8_ = 0;  //!< HAL 所用的 8 位形式从机地址（7 位地址 << 1）
        Config             cfg_{};      //!< 当前配置
        uint8_t            version_ = 0;  //!< DEVICE_ID.VER[1:0]
    };

}  // namespace Sensor
