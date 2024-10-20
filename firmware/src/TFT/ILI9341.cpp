#include "Arduino.h"
#include "ILI9341.h"
#include "esp_log.h"

#define ILI9341_NOP 0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID 0x04
#define ILI9341_RDDST 0x09

#define ILI9341_SLPIN 0x10
#define ILI9341_SLPOUT 0x11
#define ILI9341_PTLON 0x12
#define ILI9341_NORON 0x13

#define ILI9341_RDMODE 0x0A
#define ILI9341_RDMADCTL 0x0B
#define ILI9341_RDPIXFMT 0x0C
#define ILI9341_RDIMGFMT 0x0A
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF 0x20
#define ILI9341_INVON 0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON 0x29

#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C
#define ILI9341_RAMRD 0x2E

#define ILI9341_PTLAR 0x30
#define ILI9341_VSCRDEF 0x33
#define ILI9341_MADCTL 0x36
#define ILI9341_VSCRSADD 0x37
#define ILI9341_PIXFMT 0x3A

#define ILI9341_WRDISBV 0x51
#define ILI9341_RDDISBV 0x52
#define ILI9341_WRCTRLD 0x53

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR 0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_PWCTR3 0xC2
#define ILI9341_PWCTR4 0xC3
#define ILI9341_PWCTR5 0xC4
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7

#define ILI9341_RDID4 0xD3
#define ILI9341_RDINDEX 0xD9
#define ILI9341_RDID1 0xDA
#define ILI9341_RDID2 0xDB
#define ILI9341_RDID3 0xDC
#define ILI9341_RDIDX 0xDD // TBC

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_MADCTL_MY 0x80
#define ILI9341_MADCTL_MX 0x40
#define ILI9341_MADCTL_MV 0x20
#define ILI9341_MADCTL_ML 0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH 0x04

ILI9341::ILI9341(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height)
    : TFTDisplay(mosi, clk, cs, dc, rst, bl, width, height)
{
    // Initialize SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = -1,
        .sclk_io_num = clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 65535,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        //.isr_cpu_id = ESP_INTR_CPU_AFFINITY_1,
        .intr_flags = 0,
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        //.clock_source = SPI_CLK_SRC_DEFAULT,                   // Use the same frequency as the APB bus
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // Clock out at 80 MHz
        .input_delay_ns = 0,
        .spics_io_num = cs, // CS pin
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = 1,
        .pre_cb = nullptr,
        .post_cb = nullptr

    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    spi_device_acquire_bus(spi, portMAX_DELAY);

    // Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    SEND_CMD_DATA(0xEF, 0x03, 0x80, 0x02);
    SEND_CMD_DATA(0xCF, 0x00, 0xC1, 0x30);
    SEND_CMD_DATA(0xED, 0x64, 0x03, 0x12, 0x81);
    SEND_CMD_DATA(0xE8, 0x85, 0x00, 0x78);
    SEND_CMD_DATA(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
    SEND_CMD_DATA(0xF7, 0x20);
    SEND_CMD_DATA(0xEA, 0x00, 0x00);
    SEND_CMD_DATA(ILI9341_PWCTR1, 0x23);                                                                                      // Power control, VRH[5:0]
    SEND_CMD_DATA(ILI9341_PWCTR2, 0x10);                                                                                      // Power control, SAP[2:0], BT[3:0]
    SEND_CMD_DATA(ILI9341_VMCTR1, 0x3E, 0x28);                                                                                // VCM control
    SEND_CMD_DATA(ILI9341_VMCTR2, 0x86);                                                                                      // VCM control2
    SEND_CMD_DATA(ILI9341_MADCTL, ILI9341_MADCTL_MX | ILI9341_MADCTL_RGB);                                                    // Rotation 0 (portrait mode)
    SEND_CMD_DATA(ILI9341_PIXFMT, 0x55);                                                                                      // Pixel Format Set
    SEND_CMD_DATA(ILI9341_FRMCTR1, 0x00, 0x13);                                                                               // Frame Rate Control (100Hz)
    SEND_CMD_DATA(ILI9341_DFUNCTR, 0x08, 0x82, 0x27);                                                                         // Display Function Control
    SEND_CMD_DATA(0xF2, 0x00);                                                                                                // 3Gamma Function Disable
    SEND_CMD_DATA(ILI9341_GAMMASET, 0x01);                                                                                    // Gamma curve selected
    SEND_CMD_DATA(ILI9341_GMCTRP1, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); // Set Gamma
    SEND_CMD_DATA(ILI9341_GMCTRN1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); // Set Gamma
    SEND_CMD_DATA(CMD_MADCTL, MADCTL_ML | MADCTL_MV | MADCTL_BGR);
    SEND_CMD_DATA(0x11); // Exit Sleep
    delay(120);
    SEND_CMD_DATA(0x29); // Display on
    delay(120);
    fillScreen(TFT_GREEN);

    Serial.println("ILI9341 initialized");
}

ILI9341::~ILI9341()
{
}
