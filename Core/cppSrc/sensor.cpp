/**
 * @file    sensor.cpp
 * @brief   TMAG5273 三轴霍尔磁传感器驱动实现（寄存器定义见 sensor.hpp）
 */

#include "sensor.hpp"

namespace Sensor
{

    namespace
    {
        /** MAG_CH_EN 取值 → 使能的磁通道数量（表 8-5，Ch ~ Fh 为保留值）。 */
        constexpr uint8_t kMagChannelCount[16] = {
            0, 1, 1, 2, 1, 2, 2, 3, 3, 3, 3, 3, 0, 0, 0, 0,
        };

        /**
         * 一次完整转换所需时间（µs），索引 [CONV_AVG][磁通道数 - 1]。
         * 由 6.3.6 节表 6-4 的三轴更新速率换算而来；32 次平均的数值同时与
         * 5.11 节表下注(3)（825 µs + 每增加一个通道 800 µs）一致。
         */
        constexpr uint16_t kConversionTimeUs[6][3] = {
            { 50,   75,  100 },  // 1x  平均，10.0 kSPS
            { 75,  125,  175 },  // 2x  平均， 5.7 kSPS
            { 125,  227,  322 },  // 4x  平均， 3.1 kSPS
            { 227,  417,  625 },  // 8x  平均， 1.6 kSPS
            { 417,  833, 1250 },  // 16x 平均， 0.8 kSPS
            { 833, 1667, 2425 },  // 32x 平均， 0.4 kSPS
        };

        /** 从睡眠模式唤醒到可接受 I2C 命令的等待时间：Tstart_sleep 典型 50 µs，留足裕量。 */
        constexpr uint32_t kWakeUpDelayMs = TMAG5273_WAKEUP_DELAY_MS;

        /** 触发模式下等待转换完成时，在估算时间上追加的裕量。 */
        constexpr uint32_t kConversionMarginUs = TMAG5273_CONV_MARGIN_US;
    }  // namespace

    /* ========================================================================== */
    /*                                  构造函数                                   */
    /* ========================================================================== */
    TMAG5273::TMAG5273(I2C_HandleTypeDef* hi2c, uint8_t addr7)
        : hi2c_(hi2c), addr8_(static_cast<uint16_t>(addr7 << 1)), cfg_{}, version_(0) {
        cfg_.addr = addr7;
    }

    /* ========================================================================== */
    /*                                  初始化                                     */
    /* ========================================================================== */
    bool TMAG5273::init(const Config& cfg) {
        cfg_ = cfg;
        addr8_ = static_cast<uint16_t>(cfg_.addr << 1);

        /* 上电后器件先进入待机模式（Tstart_power_up 典型 270 µs），
         * 若此前被置为睡眠模式则需先唤醒。 */
        wakeUp();

        uint16_t id = 0;
        if (!manufacturerId(id) || id != kManufacturerId) {
            return false;
        }
        if (!readVersion(version_)) {  // 同时刷新 version_，供 fullScaleMt() 使用
            return false;
        }

        /* 先写各配置寄存器，最后写 DEVICE_CONFIG_2 使工作模式生效（7.2.1.2 节推荐顺序）。 */
        return applyConfig();
    }

    bool TMAG5273::applyConfig() {
        bool ok = writeConfig1();
        ok = writeSensorConfig1() && ok;
        ok = writeSensorConfig2() && ok;
        ok = writeTConfig() && ok;
        ok = writeConfig2() && ok;  // 工作模式最后生效
        return ok;
    }

    void TMAG5273::wakeUp() {
        /* 睡眠模式下从机不会回 ACK，因此本条空事务仅用于唤醒器件（5.11 节表下注(1)）。 */
        uint8_t dummy = 0;
        (void)HAL_I2C_Mem_Read(hi2c_, addr8_, static_cast<uint16_t>(Reg::DeviceId),
            I2C_MEMADD_SIZE_8BIT, &dummy, 1, kI2CTimeoutMs);
        HAL_Delay(kWakeUpDelayMs);
    }

    bool TMAG5273::isConnected() {
        uint16_t id = 0;
        return manufacturerId(id) && (id == kManufacturerId);
    }

    bool TMAG5273::manufacturerId(uint16_t& id) {
        uint8_t buf[2] = { 0, 0 };
        if (!readRegs(Reg::ManufacturerIdLsb, buf, sizeof(buf))) {
            return false;
        }
        id = static_cast<uint16_t>((static_cast<uint16_t>(buf[1]) << 8) | buf[0]);
        return true;
    }

    bool TMAG5273::readVersion(uint8_t& version) {
        uint8_t value = 0;
        if (!readReg(Reg::DeviceId, value)) {
            return false;
        }
        version = getField(value, Bits::kVersion, 2);
        version_ = version;
        return true;
    }

    float TMAG5273::fullScaleMt(uint8_t axis) const {
        /* 表 6-3：VER = 1 为 ±40/±80 mT 型号，VER = 2 为 ±133/±266 mT 型号。 */
        const bool highRange = (axis == 2) ? (cfg_.zRange == Range::High) : (cfg_.xyRange == Range::High);
        if (version_ == 2) {
            return highRange ? 266.0f : 133.0f;
        }
        return highRange ? 80.0f : 40.0f;
    }

    /* ========================================================================== */
    /*                              配置寄存器写入                                 */
    /* ========================================================================== */
    bool TMAG5273::writeConfig1() {
        uint8_t value = 0;
        value = setField(value, Bits::kCrcEn, 1, 0);   // CRC 关闭（复位默认值）
        value = setField(value, Bits::kMagTempco, 2, static_cast<uint8_t>(cfg_.magTempco & 0x03u));
        value = setField(value, Bits::kConvAvg, 3, static_cast<uint8_t>(cfg_.avg));
        value = setField(value, Bits::kI2CRd, 2, 0);   // 标准 3 字节读命令
        return writeReg(Reg::DeviceConfig1, value);
    }

    bool TMAG5273::writeConfig2() {
        uint8_t value = 0;
        value = setField(value, Bits::kThrHyst, 3, 0);  // 阈值采用补码形式（单侧阈值）
        value = setField(value, Bits::kLpLn, 1, cfg_.lowNoise ? 1u : 0u);
        value = setField(value, Bits::kI2CGlitchFilt, 1, 0);  // 毛刺滤波开启
        value = setField(value, Bits::kTriggerMode, 1, 0);    // I2C 命令触发
        value = setField(value, Bits::kOperatingMode, 2, static_cast<uint8_t>(cfg_.mode));
        return writeReg(Reg::DeviceConfig2, value);
    }

    bool TMAG5273::writeSensorConfig1() {
        uint8_t value = 0;
        value = setField(value, Bits::kMagChEn, 4, static_cast<uint8_t>(cfg_.channels));
        value = setField(value, Bits::kSleepTime, 4, static_cast<uint8_t>(cfg_.sleepTime));
        return writeReg(Reg::SensorConfig1, value);
    }

    bool TMAG5273::writeSensorConfig2() {
        uint8_t value = 0;
        value = setField(value, Bits::kAngleEn, 2, static_cast<uint8_t>(cfg_.angleEn));
        value = setField(value, Bits::kXYRange, 1, static_cast<uint8_t>(cfg_.xyRange));
        value = setField(value, Bits::kZRange, 1, static_cast<uint8_t>(cfg_.zRange));
        return writeReg(Reg::SensorConfig2, value);
    }

    bool TMAG5273::writeTConfig() {
        return writeReg(Reg::TConfig, cfg_.tempEn ? 0x01u : 0x00u);
    }

    /* ========================================================================== */
    /*                                运行期配置                                   */
    /* ========================================================================== */
    bool TMAG5273::setOperatingMode(OpMode mode) {
        cfg_.mode = mode;
        return writeConfig2();
    }

    bool TMAG5273::setChannels(MagChEn channels) {
        cfg_.channels = channels;
        return writeSensorConfig1();
    }

    bool TMAG5273::setRanges(Range xy, Range z) {
        cfg_.xyRange = xy;
        cfg_.zRange = z;
        return writeSensorConfig2();
    }

    bool TMAG5273::setAveraging(ConvAvg avg) {
        cfg_.avg = avg;
        return writeConfig1();
    }

    bool TMAG5273::setAngleChannel(AngleEn angleEn) {
        cfg_.angleEn = angleEn;
        return writeSensorConfig2();
    }

    bool TMAG5273::enableTemperature(bool enable) {
        cfg_.tempEn = enable;
        return writeTConfig();
    }

    bool TMAG5273::setI2CAddress(uint8_t addr7) {
        /* 8.1.13 节：寄存器 bit7 ~ bit1 为 7 位地址，bit0 置 1 后新地址立即生效。 */
        const uint8_t value = static_cast<uint8_t>((addr7 << 1) | 0x01u);
        if (!writeReg(Reg::I2CAddress, value)) {
            return false;
        }
        addr8_ = static_cast<uint16_t>(addr7 << 1);
        cfg_.addr = addr7;
        return true;
    }

    /* ========================================================================== */
    /*                                转换与状态                                   */
    /* ========================================================================== */
    bool TMAG5273::trigger() {
        /* 6.5.1.3.1 节：寄存器地址字节的 bit7 为转换触发位。
         * 写 CONV_STATUS 的 0x00：POR / 错误标志均为「写 1 清除」，写 0 不产生任何副作用。 */
        const uint8_t regAddr = static_cast<uint8_t>(static_cast<uint8_t>(Reg::ConvStatus) |
            (1u << Bits::kTriggerBit));
        return writeReg(static_cast<Reg>(regAddr), 0x00);
    }

    bool TMAG5273::dataReady(bool& ready) {
        uint8_t status = 0;
        if (!readConvStatus(status)) {
            return false;
        }
        ready = (getField(status, Bits::kResultStatus, 1) != 0);
        return true;
    }

    bool TMAG5273::waitDataReady(uint32_t timeoutMs) {
        const uint32_t start = HAL_GetTick();
        bool ready = false;
        do {
            if (!dataReady(ready)) {
                return false;
            }
            if (ready) {
                return true;
            }
        } while ((HAL_GetTick() - start) < timeoutMs);
        return false;
    }

    uint32_t TMAG5273::conversionTimeUs() const {
        const uint8_t channels = magChannelCount();
        uint32_t total = 0;

        if (channels > 0) {
            total = kConversionTimeUs[static_cast<uint8_t>(cfg_.avg)][channels - 1];
        }
        /* 5.11 节注(2)：CONV_AVG = 000b 时转换时间与 T_CH_EN 无关；
         * 其余平均档位按一个单通道的时间追加温度通道开销。 */
        if (cfg_.tempEn && (cfg_.avg != ConvAvg::X1)) {
            total += kConversionTimeUs[static_cast<uint8_t>(cfg_.avg)][0];
        }
        return (total == 0) ? 0 : (total + kConversionMarginUs);
    }

    uint8_t TMAG5273::magChannelCount() const {
        return kMagChannelCount[static_cast<uint8_t>(cfg_.channels) & 0x0Fu];
    }

    /* ========================================================================== */
    /*                                  结果读取                                   */
    /* ========================================================================== */
    bool TMAG5273::readSample(Sample& sample) {
        /* 0x10 ~ 0x1B 一次连续读出：T、X、Y、Z、CONV_STATUS、ANGLE、MAGNITUDE。 */
        uint8_t buf[kResultBlockLen] = { 0 };
        if (!readRegs(Reg::TMsbResult, buf, kResultBlockLen)) {
            return false;
        }

        const uint8_t status = buf[8];  // 0x18 CONV_STATUS

        sample.tRaw = static_cast<int16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
        sample.xRaw = static_cast<int16_t>((static_cast<uint16_t>(buf[2]) << 8) | buf[3]);
        sample.yRaw = static_cast<int16_t>((static_cast<uint16_t>(buf[4]) << 8) | buf[5]);
        sample.zRaw = static_cast<int16_t>((static_cast<uint16_t>(buf[6]) << 8) | buf[7]);
        sample.angleRaw = static_cast<uint16_t>((static_cast<uint16_t>(buf[9]) << 8) | buf[10]);
        sample.magnitude = buf[11];

        sample.t = rawToCelsius(sample.tRaw);
        sample.x = rawToMilliTesla(sample.xRaw, fullScaleMt(0));
        sample.y = rawToMilliTesla(sample.yRaw, fullScaleMt(1));
        sample.z = rawToMilliTesla(sample.zRaw, fullScaleMt(2));
        sample.angle = rawToDegree(sample.angleRaw);

        sample.ready = (getField(status, Bits::kResultStatus, 1) != 0);
        sample.diagFault = (getField(status, Bits::kDiagStatus, 1) != 0);
        sample.por = (getField(status, Bits::kPor, 1) != 0);
        return true;
    }

    bool TMAG5273::readMagRaw(int16_t& x, int16_t& y, int16_t& z) {
        uint8_t buf[6] = { 0 };
        if (!readRegs(Reg::XMsbResult, buf, sizeof(buf))) {
            return false;
        }
        x = static_cast<int16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
        y = static_cast<int16_t>((static_cast<uint16_t>(buf[2]) << 8) | buf[3]);
        z = static_cast<int16_t>((static_cast<uint16_t>(buf[4]) << 8) | buf[5]);
        return true;
    }

    bool TMAG5273::readMag(float& x, float& y, float& z) {
        int16_t rawX = 0;
        int16_t rawY = 0;
        int16_t rawZ = 0;
        if (!readMagRaw(rawX, rawY, rawZ)) {
            return false;
        }
        x = rawToMilliTesla(rawX, fullScaleMt(0));
        y = rawToMilliTesla(rawY, fullScaleMt(1));
        z = rawToMilliTesla(rawZ, fullScaleMt(2));
        return true;
    }

    bool TMAG5273::readTemperature(float& celsius) {
        uint8_t buf[2] = { 0, 0 };
        if (!readRegs(Reg::TMsbResult, buf, sizeof(buf))) {
            return false;
        }
        const int16_t raw = static_cast<int16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
        celsius = rawToCelsius(raw);
        return true;
    }

    bool TMAG5273::readAngle(float& degree) {
        uint8_t buf[2] = { 0, 0 };
        if (!readRegs(Reg::AngleResultMsb, buf, sizeof(buf))) {
            return false;
        }
        degree = rawToDegree(static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]));
        return true;
    }

    bool TMAG5273::readConvStatus(uint8_t& status) {
        return readReg(Reg::ConvStatus, status);
    }

    bool TMAG5273::readDeviceStatus(uint8_t& status) {
        return readReg(Reg::DeviceStatus, status);
    }

    bool TMAG5273::clearDeviceErrors(uint8_t mask) {
        /* 8.1.29 节：错误标志为 R/W1CP，写入 1 清除。 */
        return writeReg(Reg::DeviceStatus, static_cast<uint8_t>(mask & 0x0Fu));
    }

    /* ========================================================================== */
    /*                                  底层访问                                   */
    /* ========================================================================== */
    bool TMAG5273::readRegs(Reg start, uint8_t* data, uint16_t len) {
        if ((data == nullptr) || (len == 0)) {
            return false;
        }
        /* HAL_I2C_Mem_Read 产生「START + 地址(W) + 寄存器地址，RESTART + 地址(R) + 数据」时序，
         * 与 6.5.1.3.3 节图 6-9 的标准 3 字节读命令一致。 */
        return HAL_I2C_Mem_Read(hi2c_, addr8_, static_cast<uint16_t>(start), I2C_MEMADD_SIZE_8BIT,
            data, len, kI2CTimeoutMs) == HAL_OK;
    }

    bool TMAG5273::readReg(Reg reg, uint8_t& value) {
        return readRegs(reg, &value, 1);
    }

    bool TMAG5273::writeReg(Reg reg, uint8_t value) {
        return HAL_I2C_Mem_Write(hi2c_, addr8_, static_cast<uint16_t>(reg), I2C_MEMADD_SIZE_8BIT,
            &value, 1, kI2CTimeoutMs) == HAL_OK;
    }

}  // namespace Sensor
