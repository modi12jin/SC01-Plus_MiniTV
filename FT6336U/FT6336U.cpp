#include "FT6336U.h"

FT6336U::FT6336U(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin)
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
}

void IRAM_ATTR FT6336U::handleISR(void) {
  _state = true;
}

void FT6336U::begin(void)
{
    // Initialize I2C
    if (_sda != -1 && _scl != -1)
    {
        Wire.begin(_sda, _scl);
    }
    else
    {
        Wire.begin();
    }

    // Int Pin Configuration
    if (_int != -1)
    {
        pinMode(_int, INPUT_PULLUP);
        attachInterrupt(_int, std::bind(&FT6336U::handleISR, this), FALLING);
    }

    // Reset Pin Configuration
    if (_rst != -1)
    {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(10);
        digitalWrite(_rst, HIGH);
        delay(300);
    }

    // Initialize Touch
    i2c_write(0x86, 0X00); //禁止进入监控模式。
    i2c_write(0xD0, 0X01); //启动手势。
}

bool FT6336U::getTouch(uint16_t *x, uint16_t *y)
{
    bool FingerIndex = false;
    FingerIndex = (bool)i2c_read(0x02);

    uint8_t data[4];
    i2c_read_continuous(0x03, data, 4);
    *x = ((data[0] & 0x0f) << 8) | data[1];
    *y = ((data[2] & 0x0f) << 8) | data[3];

    return FingerIndex;
}

bool FT6336U::getGesture(uint8_t *gesture)
{
    if (_state)
    {

        *gesture = i2c_read(0xD3);

        _state = false;
        return true;
    }

    return false;
}

uint8_t FT6336U::i2c_read(uint8_t addr)
{
    uint8_t rdData;
    uint8_t rdDataCount;
    do
    {
        Wire.beginTransmission(I2C_ADDR_FT6336U);
        Wire.write(addr);
        Wire.endTransmission(false); // Restart
        rdDataCount = Wire.requestFrom(I2C_ADDR_FT6336U, 1);
    } while (rdDataCount == 0);
    while (Wire.available())
    {
        rdData = Wire.read();
    }
    return rdData;
}

uint8_t FT6336U::i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length)
{
    Wire.beginTransmission(I2C_ADDR_FT6336U);
    Wire.write(addr);
    if (Wire.endTransmission(true))
        return -1;
    Wire.requestFrom(I2C_ADDR_FT6336U, length);
    for (int i = 0; i < length; i++)
    {
        *data++ = Wire.read();
    }
    return 0;
}

void FT6336U::i2c_write(uint8_t addr, uint8_t data)
{
    Wire.beginTransmission(I2C_ADDR_FT6336U);
    Wire.write(addr);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t FT6336U::i2c_write_continuous(uint8_t addr, const uint8_t *data, uint32_t length)
{
    Wire.beginTransmission(I2C_ADDR_FT6336U);
    Wire.write(addr);
    for (int i = 0; i < length; i++)
    {
        Wire.write(*data++);
    }
    if (Wire.endTransmission(true))
        return -1;
    return 0;
}
