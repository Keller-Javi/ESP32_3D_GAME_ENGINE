#ifndef GFX_CONFIG
#define GFX_CONFIG

#include <LovyanGFX.hpp>

#include "engine_config.h"

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI _bus;

public:

    LGFX()
    {
        {
            auto cfg = _bus.config();

            cfg.spi_host = SPI2_HOST;      // ESP32-S3
            cfg.spi_mode = 0;

            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.freq_write = 80000000;
            cfg.freq_read  = 16000000;

            cfg.pin_sclk = 12;
            cfg.pin_mosi = 11;
            cfg.pin_miso = 13;

            cfg.pin_dc = 7;

            cfg.use_lock = true;

            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        {
            auto cfg = _panel.config();

            cfg.pin_cs  = 10;
            cfg.pin_rst = 6;
            cfg.pin_busy = -1;

            cfg.panel_width  = WIDTH;
            cfg.panel_height = HEIGHT;

            cfg.offset_x = 0;
            cfg.offset_y = 0;

            cfg.invert = false;
            cfg.rgb_order = false;

            _panel.config(cfg);
        }

        setPanel(&_panel);
    }
};

#endif