#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "SDCard.h"

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO

SDCard::SDCard(const char *mountPoint, gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3)
{
  #ifdef USE_SDIO
  m_mountPoint = mountPoint;
  m_host.max_freq_khz = SDMMC_FREQ_52M;
  m_host.flags = SDMMC_HOST_FLAG_4BIT;
  esp_err_t ret;
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  Serial.println("Initializing SD card");

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
  slot_config.width = 4;
  slot_config.clk = clk;
  slot_config.cmd = cmd;
  slot_config.d0 = d0;
  slot_config.d1 = d1;
  slot_config.d2 = d2;
  slot_config.d3 = d3;
  slot_config.d4 = GPIO_NUM_NC;
  slot_config.d5 = GPIO_NUM_NC;
  slot_config.d6 = GPIO_NUM_NC;
  slot_config.d7 = GPIO_NUM_NC;
  ret = esp_vfs_fat_sdmmc_mount(mountPoint, &m_host, &slot_config, &mount_config, &m_card);
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      Serial.println("Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      Serial.printf("Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.\n",
               esp_err_to_name(ret));
    }
    return;
  }
  Serial.printf("SDCard mounted at: %s\n", mountPoint);
  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, m_card);
  _isMounted = true;
  #endif
}

SDCard::SDCard(const char *mountPoint, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs)
{
  m_mountPoint = mountPoint;
  m_host.max_freq_khz = SDMMC_FREQ_52M;
  // only enable on ESP32 
  #ifdef SD_CARD_SPI_HOST
  m_host.slot = SD_CARD_SPI_HOST;
  #endif
  esp_err_t ret;
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  Serial.println("Initializing SD card");

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = mosi,
      .miso_io_num = miso,
      .sclk_io_num = clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 16384,
      .flags = 0,
      .intr_flags = 0
  };
  ret = spi_bus_initialize(spi_host_device_t(m_host.slot), &bus_cfg, SPI_DMA_CHAN);
  if (ret != ESP_OK)
  {
    Serial.println("Failed to initialize bus.");
    return;
  }

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = cs;
  slot_config.host_id = spi_host_device_t(m_host.slot);

  ret = esp_vfs_fat_sdspi_mount(mountPoint, &m_host, &slot_config, &mount_config, &m_card);
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      Serial.println("Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      Serial.printf("Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.\n",
               esp_err_to_name(ret));
    }
    return;
  }
  Serial.printf("SDCard mounted at: %s\n", mountPoint);
  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, m_card);
  _isMounted = true;
}

SDCard::~SDCard()
{
  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdcard_unmount(m_mountPoint.c_str(), m_card);
  //deinitialize the bus after all devices are removed
  spi_bus_free(spi_host_device_t(m_host.slot));
}
