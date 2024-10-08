#pragma once

#include <driver/sdmmc_types.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>

class SDCard
{
private:
  bool _isMounted = false;
  std::string m_mountPoint;
  sdmmc_card_t *m_card;
  #ifdef USE_SDIO
    sdmmc_host_t m_host = SDMMC_HOST_DEFAULT();
  #else
    sdmmc_host_t m_host = SDSPI_HOST_DEFAULT();
  #endif
public:
  bool isMounted() { return _isMounted; }
  const char *mountPoint() {
    return m_mountPoint.c_str();
  }
  SDCard(const char *mountPoint, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs);
  SDCard(const char *mountPoint, gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3);
  ~SDCard();
};