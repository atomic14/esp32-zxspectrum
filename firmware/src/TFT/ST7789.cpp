#include "Arduino.h"
#include "ST7789.h"
#include "esp_log.h"

ST7789::ST7789(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height)
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
        .clock_speed_hz = TFT_SPI_FREQUENCY,
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

    SEND_CMD_DATA(0x11);
    vTaskDelay(pdMS_TO_TICKS(255));
    SEND_CMD_DATA(0x3a, 0x55);
    SEND_CMD_DATA(0x36, 0x00);
    SEND_CMD_DATA(0x2a, 0x00, 0x00, 0x00, 0xf0);
    SEND_CMD_DATA(0x2b, 0x00, 0x00, 0x00, 0xf0);
    SEND_CMD_DATA(0x21);
    vTaskDelay(pdMS_TO_TICKS(10));
    SEND_CMD_DATA(0x13);
    vTaskDelay(pdMS_TO_TICKS(10));
    SEND_CMD_DATA(0x29);
    SEND_CMD_DATA(CMD_MADCTL, MADCTL_MX | MADCTL_MV | MADCTL_RGB);
    Serial.println("ST7789 initialized");
}

ST7789::~ST7789()
{
}
