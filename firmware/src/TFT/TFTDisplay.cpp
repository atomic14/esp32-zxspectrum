#include <Arduino.h>
#include "Serial.h"
#include "TFTDisplay.h"
#include <cstring>
#include <algorithm>
#include <vector>


// max buffer size that we support - larger transfers will be split accross multiple transactions
const size_t DMA_BUFFER_SIZE = 32768;

class SPITransactionInfo
{
public:
  spi_transaction_t transaction;
  bool isCommand = false;    // false = data, true = command
  void *buffer = nullptr;    // the DMA buffer
  TFTDisplay *display = nullptr; // the display that the transaction was for

  SPITransactionInfo(size_t bufferSize)
  {
    memset(&transaction, 0, sizeof(transaction));
    transaction.user = this;
    if (bufferSize > 0)
    {
      buffer = heap_caps_malloc(bufferSize, MALLOC_CAP_DMA);
    }
  }

  void setCommand(uint8_t cmd)
  {
    memset(&transaction, 0, sizeof(transaction));
    isCommand = true;
    transaction.length = 8; // Command is 8 bits
    transaction.tx_data[0] = cmd;
    transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    transaction.user = this;
  }

  bool setData(const uint8_t *data, int len)
  {
    memset(&transaction, 0, sizeof(transaction));
    isCommand = false;
    transaction.length = len * 8; // Data length in bits
    transaction.user = this;
    if (len <= 4)
    {
      for (int i = 0; i < len; i++)
      {
        transaction.tx_data[i] = data[i];
      }
      transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    }
    else
    {
      memcpy(buffer, data, len);
      transaction.tx_buffer = buffer;
    }
    return true;
  }

  bool setPixels(const uint16_t *data, int numPixels)
  {
    return setData((const uint8_t *)data, numPixels * 2);
  }

  bool setColor(uint16_t color, int numPixels)
  {
    uint16_t *pixels = (uint16_t *)buffer;
    for (int i = 0; i < numPixels; i++)
    {
      pixels[i] = color;
    }
    memset(&transaction, 0, sizeof(transaction));
    isCommand = false;
    transaction.length = numPixels * 16; // Data length in bits
    transaction.tx_buffer = buffer;
    transaction.user = this;
    return true;
  }
};

TFTDisplay::TFTDisplay(gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height)
    : Display(width, height), cs(cs), dc(dc), rst(rst), bl(bl), spi(nullptr)
{
  mDisplayLock = xSemaphoreCreateBinary();
  xSemaphoreGive(mDisplayLock);
  pinMode(rst, OUTPUT);
  pinMode(dc, OUTPUT);
  if (bl != GPIO_NUM_NC)
  {
    gpio_set_direction(bl, GPIO_MODE_OUTPUT);
    gpio_set_level(bl, 1); // Turn on backlight
  }

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

  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

  _transaction = new SPITransactionInfo(DMA_BUFFER_SIZE);
  Serial.println("TFTDisplay created");
}

void TFTDisplay::sendCmd(uint8_t cmd)
{
  dmaWait();
  _transaction->setCommand(cmd);
  sendTransaction(_transaction);
}

void TFTDisplay::sendTransaction(SPITransactionInfo *trans)
{
  isBusy = true;
  if (trans->isCommand)
  {
    digitalWrite(dc, LOW);
    // gpio_set_level(dc, 0); // Command mode
  }
  else
  {
    digitalWrite(dc, HIGH);
    // gpio_set_level(dc, 1); // Data mode
  }
  spi_device_polling_start(spi, &trans->transaction, portMAX_DELAY);
}

void TFTDisplay::sendPixels(const uint16_t *data, int numPixels)
{
  int bytes = numPixels * 2;
  for (uint32_t i = 0; i < bytes; i += DMA_BUFFER_SIZE)
  {
    uint32_t len = std::min(DMA_BUFFER_SIZE, bytes - i);
    dmaWait();
    _transaction->setPixels(data + i / 2, len / 2);
    sendTransaction(_transaction);
  }
}

void TFTDisplay::sendData(const uint8_t *data, int length)
{
  for (uint32_t i = 0; i < length; i += DMA_BUFFER_SIZE)
  {
    uint32_t len = std::min(DMA_BUFFER_SIZE, length - i);
    dmaWait();
    _transaction->setData(data + i, len);
    sendTransaction(_transaction);
  }
}

void TFTDisplay::sendColor(uint16_t color, int numPixels)
{
  for (int i = 0; i < numPixels; i += DMA_BUFFER_SIZE >> 1)
  {
    int len = std::min(numPixels - i, int(DMA_BUFFER_SIZE >> 1));
    dmaWait();
    _transaction->setColor(color, len);
    sendTransaction(_transaction);
  }
}

void TFTDisplay::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  // do the TFT window set
  uint8_t data[4];
  #ifdef TFT_X_OFFSET
  x0+=TFT_X_OFFSET;
  x1+=TFT_X_OFFSET;
  #endif

  sendCmd(ST7789_CMD_CASET); // Column address set
  data[0] = (x0 >> 8) & 0xFF;
  data[1] = x0 & 0xFF;
  data[2] = (x1 >> 8) & 0xFF;
  data[3] = x1 & 0xFF;
  sendData(data, 4);

  sendCmd(ST7789_CMD_RASET); // Row address set
  data[0] = (y0 >> 8) & 0xFF;
  data[1] = y0 & 0xFF;
  data[2] = (y1 >> 8) & 0xFF;
  data[3] = y1 & 0xFF;
  sendData(data, 4);

  sendCmd(ST7789_CMD_RAMWR); // Write to RAM
}

void TFTDisplay::sendPixel(uint16_t color)
{
  uint16_t data[1];
  data[0] = color;
  sendPixels(data, 1);
}

void TFTDisplay::dmaWait()
{
  if (isBusy)
  {
    spi_device_polling_end(spi, portMAX_DELAY);
    isBusy = false;
  }
}
