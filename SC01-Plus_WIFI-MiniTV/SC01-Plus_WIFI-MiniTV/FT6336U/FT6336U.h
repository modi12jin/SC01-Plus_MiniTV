#ifndef _FT6336U_H
#define _FT6336U_H

#include <Wire.h>
#include <FunctionalInterrupt.h>

#define I2C_ADDR_FT6336U 0x38

//手势
enum GESTURE  {
    None = 0x00,//无手势
    SlideDown = 0x23,//向下滑动
    SlideUp = 0x22,//向上滑动
    SlideLeft = 0x20,//向左滑动
    SlideRight = 0x21,//向右滑动
    DoubleTap = 0x24,//双击
};

/**************************************************************************/
/*!
    @brief  FT6336U I2C CTP controller driver
*/
/**************************************************************************/
class FT6336U 
{
public:
    FT6336U(int8_t sda_pin=-1, int8_t scl_pin=-1, int8_t rst_pin=-1, int8_t int_pin=-1); 

    void begin(void);
    bool getTouch(uint16_t *x, uint16_t *y);
    bool getGesture(uint8_t *gesture);

private: 
    int8_t _sda,_scl,_rst,_int;
    bool _state;

    void IRAM_ATTR handleISR();

    uint8_t i2c_read(uint8_t addr);
    uint8_t i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length);
    void i2c_write(uint8_t addr, uint8_t data);
    uint8_t i2c_write_continuous(uint8_t addr, const uint8_t *data, uint32_t length); 
}; 
#endif