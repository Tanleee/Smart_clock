#pragma once
#include <LovyanGFX.hpp>

#define TFT_CS      4
#define TFT_RST     8
#define TFT_DC      3
#define TFT_MOSI    7
#define TFT_CLK     6

#define TOUCH_CS    0    
#define TOUCH_DO    10   

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789    _panel_instance;
  lgfx::Bus_SPI         _bus_instance;
  lgfx::Touch_XPT2046   _touch_instance;

public:
  LGFX(void)
  {
    { // Cấu hình SPI bus — thêm pin_miso vì giờ cần đọc dữ liệu cảm ứng
      auto cfg = _bus_instance.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_CLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TOUCH_DO;   /* đổi từ -1 sang chân thật */
      cfg.pin_dc   = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Cấu hình panel — giữ nguyên như cũ (đã sửa rgb_order ở bước trước)
      auto cfg = _panel_instance.config();
      cfg.pin_cs  = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.readable  = false;
      cfg.invert    = false;
      cfg.rgb_order = true;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }

    { // Cấu hình touch XPT2046 — dùng chung bus SPI với màn hình
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;    cfg.x_max = 4095;   /* giá trị ADC thô, sẽ hiệu chỉnh lại ở bước sau */
      cfg.y_min = 0;    cfg.y_max = 4095;
      cfg.pin_int    = -1;        /* không dùng IRQ, đọc kiểu polling */
      cfg.bus_shared = true;      /* dùng chung bus SPI với panel */
      cfg.spi_host   = SPI2_HOST;
      cfg.freq       = 1000000;
      cfg.pin_sclk   = TFT_CLK;
      cfg.pin_mosi   = TFT_MOSI;
      cfg.pin_miso   = TOUCH_DO;
      cfg.pin_cs     = TOUCH_CS;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};