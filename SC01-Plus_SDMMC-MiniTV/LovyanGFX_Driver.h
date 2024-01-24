#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {

  lgfx::Panel_ST7796 _panel_instance;

  lgfx::Bus_Parallel8 _bus_instance;

  lgfx::Light_PWM _light_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();

      cfg.port = 0;
      cfg.freq_write = 40000000;
      cfg.pin_wr = 47;  // WR を接続しているピン番号
      cfg.pin_rd = -1;  // RD を接続しているピン番号
      cfg.pin_rs = 0;   // RS(D/C)を接続しているピン番号
      cfg.pin_d0 = 9;   // D0を接続しているピン番号
      cfg.pin_d1 = 46;  // D1を接続しているピン番号
      cfg.pin_d2 = 3;   // D2を接続しているピン番号
      cfg.pin_d3 = 8;   // D3を接続しているピン番号
      cfg.pin_d4 = 18;  // D4を接続しているピン番号
      cfg.pin_d5 = 17;  // D5を接続しているピン番号
      cfg.pin_d6 = 16;  // D6を接続しているピン番号
      cfg.pin_d7 = 15;  // D7を接続しているピン番号

      _bus_instance.config(cfg);               // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);  // バスをパネルにセットします。
    }

    {                                       // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();  // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs = -1;    // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst = 4;    // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy = -1;  // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定さ BUSYが接続されているピン番号 (-1 = disable)れていますので、不明な項目はコメントアウトして試してみてください。

      cfg.panel_width = 320;     // 实际显示宽度
      cfg.panel_height = 480;    // 实际显示高度
      cfg.offset_x = 0;          // パネルのX方向オフセット量
      cfg.offset_y = 0;          // パネルのY方向オフセット量
      cfg.offset_rotation = 1;   //值在旋转方向的偏移0~7（4~7是倒置的）
      cfg.dummy_read_pixel = 8;  // 在读取像素之前读取的虚拟位数
      cfg.dummy_read_bits = 1;   // 读取像素以外的数据之前的虚拟读取位数
      cfg.readable = false;      // 如果可以读取数据，则设置为 true
      cfg.invert = true;         // 如果面板的明暗反转，则设置为 true
      cfg.rgb_order = false;     // 如果面板的红色和蓝色被交换，则设置为 true
      cfg.dlen_16bit = false;    // 对于以 16 位单位发送数据长度的面板，设置为 true
      cfg.bus_shared = false;    // 如果总线与 SD 卡共享，则设置为 true（使用 drawJpgFile 等执行总线控制）

      _panel_instance.config(cfg);
    }

    {                                       // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();  // バックライト設定用の構造体を取得します。

      cfg.pin_bl = 45;      // バックライトが接続されているピン番号
      cfg.invert = false;   // バックライトの輝度を反転させる場合 true
      cfg.freq = 44100;     // バックライトのPWM周波数
      cfg.pwm_channel = 1;  // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }

    setPanel(&_panel_instance);  // 使用するパネルをセットします。
  }
};
