#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "Serial.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "SDCard.h"
#include "ff.h"      // FatFS header for f_getfree
#include "diskio.h"  // FatFS disk I/O

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

  m_sector_count = m_card->csd.capacity;
  m_sector_size = m_card->csd.sector_size;

  m_mutex = xSemaphoreCreateMutex();
  #endif
}

SDCard::SDCard(const char *mountPoint, gpio_num_t cs) {
  m_mountPoint = mountPoint;
  m_host.max_freq_khz = SDMMC_FREQ_52M;
  esp_err_t ret;
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  Serial.println("Initializing SD card");

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

  m_sector_count = m_card->csd.capacity;
  m_sector_size = m_card->csd.sector_size;
  
  m_mutex = xSemaphoreCreateMutex();
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

  m_sector_count = m_card->csd.capacity;
  m_sector_size = m_card->csd.sector_size;
  
  m_mutex = xSemaphoreCreateMutex();
}

SDCard::~SDCard()
{
  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdcard_unmount(m_mountPoint.c_str(), m_card);
  //deinitialize the bus after all devices are removed
  spi_bus_free(spi_host_device_t(m_host.slot));
}

bool SDCard::writeSectors(uint8_t *src, size_t start_sector, size_t sector_count) {
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  digitalWrite(GPIO_NUM_2, HIGH);
  esp_err_t res = sdmmc_write_sectors(m_card, src, start_sector, sector_count);
  digitalWrite(GPIO_NUM_2, LOW);
  xSemaphoreGive(m_mutex);
  return res == ESP_OK;
}

bool SDCard::readSectors(uint8_t *dst, size_t start_sector, size_t sector_count) {
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  digitalWrite(GPIO_NUM_2, HIGH);
  esp_err_t res = sdmmc_read_sectors(m_card, dst, start_sector, sector_count);
  digitalWrite(GPIO_NUM_2, LOW);
  xSemaphoreGive(m_mutex);
  return res == ESP_OK;
}

bool SDCard::getSpace(uint64_t &total, uint64_t &used) {
  if (!_isMounted) {
    return false;
  }
  
  // Total space calculation from sectors
  total = (uint64_t)m_sector_count * 512;
  
  // Use FatFS f_getfree to get free space information
  FATFS* fs;
  DWORD free_clusters = 0;
  char driveStr[3] = {'0', ':', 0}; // Default drive
  
  FRESULT res = f_getfree(driveStr, &free_clusters, &fs);
  if (res == FR_OK) {
    uint64_t total_clusters = (uint64_t)(fs->n_fatent - 2);
    uint64_t cluster_size = (uint64_t)(fs->csize) * 512; // Sector size is 512 bytes
    
    uint64_t total_size = total_clusters * cluster_size;
    uint64_t free_size = (uint64_t)free_clusters * cluster_size;
    
    // Sanity check
    if (total_size > 0 && free_size <= total_size) {
      // Update with calculated values for better accuracy
      total = total_size;
      used = total_size - free_size;
      return true;
    }
  }
  
  // If f_getfree fails, fall back to just reporting total space
  used = 0;
  return true;
}