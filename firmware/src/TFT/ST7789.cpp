#include "ST7789.h"
#include <cstring>
#include "freertos/task.h"
#include <freertos/queue.h>
#include "esp_log.h"
#include "../Emulator/48k_rom.h"

#define ST7789_CMD_MADCTL 0x36
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08

#define ST7789_CMD_SWRESET 0x01
#define ST7789_CMD_SLPOUT 0x11
#define ST7789_CMD_DISPON 0x29
#define ST7789_CMD_CASET 0x2A
#define ST7789_CMD_RASET 0x2B
#define ST7789_CMD_RAMWR 0x2C

const size_t NUM_TRANSACTIONS = 20;
const size_t DMA_BUFFER_SIZE = 320 * 8 * sizeof(uint16_t);


// helper functions
inline uint16_t swapBytes(uint16_t val)
{
    return (val >> 8) | (val << 8);
}

uint32_t swapEndian32(uint32_t val)
{
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8) & 0x0000FF00) |
           ((val << 8) & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

int32_t readInt32(const uint8_t *data)
{
    int32_t val = *((int32_t *)data);
    val = (int32_t)swapEndian32((uint32_t)val);
    return val;
}

uint32_t readUInt32(const uint8_t *data)
{
    uint32_t val = *((uint32_t *)data);
    val = (uint32_t)swapEndian32((uint32_t)val);
    return val;
}

class SPITransactionInfo
{
public:
    spi_transaction_t transaction;
    bool isCommand = false;    // false = data, true = command
    void *buffer = nullptr; // the DMA buffer
    size_t bufferSize = 0;
    ST7789 *display = nullptr; // the display that the transaction was for

    SPITransactionInfo()
    {
        memset(&transaction, 0, sizeof(transaction));
        transaction.user = this;
        buffer = heap_caps_malloc(DMA_BUFFER_SIZE, MALLOC_CAP_DMA);
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

    bool copyData(const uint8_t *data, int len)
    {
        memcpy(buffer, data, len);
        return true;
    }

    bool setData(const uint8_t *data, int len)
    {
        if (copyData(data, len)) {
            memset(&transaction, 0, sizeof(transaction));
            isCommand = false;
            transaction.length = len * 8; // Data length in bits
            transaction.tx_buffer = buffer;
            transaction.user = this;
            return true;
        }
        return false;
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

/*
We will have a queue of SPI transactions that can be pulled from. We'll allocate these at the start and queue them up.
We will also have a queue of DMA buffers (matching the number of transactions) these will also be pre-allocated and queued up.

When we want to send data, we will pull a transaction from the queue and a buffer from the queue. We will fill the buffer with the
data to send and the transaction with the buffer and the display instance. We will then queue the transaction.

In the pre-transmit callback we chack the SPITransactionInfo to see if it is a command or data. If it is a command we set the DC pin.
In the post-transmit callback we will return the buffer to the queue and the transaction to the queue.
 */

ST7789::ST7789(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height)
    : width(width), height(height), rotation(3), mosi(mosi), clk(clk), cs(cs), dc(dc), rst(rst), bl(bl), spi(nullptr)
{
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);
    gpio_set_direction(dc, GPIO_MODE_OUTPUT);
    if (bl != GPIO_NUM_NC)
    {
        gpio_set_direction(bl, GPIO_MODE_OUTPUT);
        gpio_set_level(bl, 1); // Turn on backlight
    }

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
        .max_transfer_sz = DMA_BUFFER_SIZE,
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
        .clock_speed_hz = SPI_MASTER_FREQ_80M, // Clock out at 80 MHz
        .input_delay_ns = 0,
        .spics_io_num = cs, // CS pin
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = NUM_TRANSACTIONS, // Queue 7 transactions at a time
        .pre_cb = spi_pre_transfer_callback,
        .post_cb = spi_post_transfer_callback,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    // setup the transaction queue
    transactionQueue = xQueueCreate(NUM_TRANSACTIONS, sizeof(SPITransactionInfo *));
    for (int i = 0; i < NUM_TRANSACTIONS; i++)
    {
        SPITransactionInfo *transaction = new SPITransactionInfo();
        if (!transaction)
        {
            Serial.println("Failed to allocate transaction");
        }
        else
        {
            transaction->display = this;
            xQueueSendToBack(transactionQueue, &transaction, portMAX_DELAY);
        }
    }
    Serial.println("Created SPI transactions");
    // Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    sendCmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(255));
    sendCmd(0x3a);
    uint8_t data[] = {0x55};
    sendData(data, 1);
    sendCmd(0x36);
    uint8_t madctl = 0x00;
    sendData(&madctl, 1);
    sendCmd(0x2a);
    uint8_t data2[] = {0x00, 0x00, 0x00, 0xf0};
    sendData(data2, 4);
    sendCmd(0x2b);
    uint8_t data3[] = {0x00, 0x00, 0x00, 0xf0};
    sendData(data3, 4);
    sendCmd(0x21);
    vTaskDelay(pdMS_TO_TICKS(10));
    sendCmd(0x13);
    vTaskDelay(pdMS_TO_TICKS(10));
    sendCmd(0x29);
    setRotation(rotation);

    Serial.println("ST7789 initialized");
}

ST7789::~ST7789()
{
}

void IRAM_ATTR ST7789::spi_post_transfer_callback(spi_transaction_t *trans)
{
    SPITransactionInfo *transaction = (SPITransactionInfo *)trans->user;
    xQueueSendFromISR(transaction->display->transactionQueue, &transaction, nullptr);
}

void IRAM_ATTR ST7789::spi_pre_transfer_callback(spi_transaction_t *trans)
{
    SPITransactionInfo *transaction = (SPITransactionInfo *)trans->user;
    if (transaction->isCommand)
    {
        gpio_set_level(transaction->display->dc, 0); // Command mode
    }
    else
    {
        gpio_set_level(transaction->display->dc, 1); // Data mode
    }
}

void ST7789::sendCmd(uint8_t cmd)
{
    SPITransactionInfo *trans;
    if (xQueueReceive(transactionQueue, &trans, portMAX_DELAY) == pdTRUE)
    {
        trans->setCommand(cmd);
        if (spi_device_queue_trans(spi, &trans->transaction, portMAX_DELAY) != ESP_OK)
        {
            Serial.println("Failed to queue transaction");
        }
    }
    else
    {
        Serial.println("Failed to get transaction");
    }
}

void ST7789::sendPixels(const uint16_t *data, int numPixels)
{
    int bytes = numPixels * 2;
    for(uint32_t i = 0; i < bytes; i += DMA_BUFFER_SIZE) {
        uint32_t len = std::min(DMA_BUFFER_SIZE, bytes - i);
        SPITransactionInfo *trans;
        if (xQueueReceive(transactionQueue, &trans, portMAX_DELAY) == pdTRUE)
        {
            trans->setPixels(data + i / 2, len / 2);
            if (spi_device_queue_trans(spi, &trans->transaction, portMAX_DELAY) != ESP_OK)
            {
                Serial.println("Failed to queue transaction");
            }
        }
        else
        {
            Serial.println("Failed to get transaction");
        }
    }
}

void ST7789::sendData(const uint8_t *data, int length)
{
    SPITransactionInfo *trans;
    if (xQueueReceive(transactionQueue, &trans, portMAX_DELAY) == pdTRUE)
    {
        trans->setData(data, length);
        if (spi_device_queue_trans(spi, &trans->transaction, portMAX_DELAY) != ESP_OK)
        {
            Serial.println("Failed to queue transaction");
        }
    }
    else
    {
        Serial.println("Failed to get transaction");
    }
}

void ST7789::sendColor(uint16_t color, int numPixels)
{
    SPITransactionInfo *trans;
    if (xQueueReceive(transactionQueue, &trans, portMAX_DELAY) == pdTRUE)
    {
        trans->setColor(color, numPixels);
        if (spi_device_queue_trans(spi, &trans->transaction, portMAX_DELAY) != ESP_OK)
        {
            Serial.println("Failed to queue transaction");
        }
    }
    else
    {
        Serial.println("Failed to get transaction");
    }
}

void ST7789::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    uint8_t data[4];

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

void ST7789::sendPixel(uint16_t color)
{
    uint16_t data[1];
    data[0] = color;
    sendPixels(data, 1);
}

void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    setWindow(x, y, x + w - 1, y + h - 1);
    // quick path - where the width x height is smaller than our DMA buffer
    if (w * h < DMA_BUFFER_SIZE / 2)
    {
        sendColor(color, w * h);
    }
    else
    {
        // work out how many lines we can send at once
        int linesPerTransaction = DMA_BUFFER_SIZE / (w * 2);
        int linesToSend = h;
        while (linesToSend > 0)
        {
            sendColor(color, w * std::min(linesPerTransaction, linesToSend));
            linesToSend -= linesPerTransaction;
        }
    }
}

void ST7789::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    fillRect(x, y, w, 1, swapBytes(color));
}

void ST7789::fillScreen(uint16_t color)
{
    fillRect(0, 0, width, height, swapBytes(color));
}

void ST7789::loadFont(const uint8_t *fontData)
{
    currentFont.fontData = fontData;
    // Read font metadata with endianness correction
    currentFont.gCount = readUInt32(fontData);
    uint32_t version = readUInt32(fontData + 4);
    uint32_t fontSize = readUInt32(fontData + 8);
    uint32_t mboxY = readUInt32(fontData + 12);
    currentFont.ascent = readUInt32(fontData + 16);
    currentFont.descent = readUInt32(fontData + 20);
    Serial.printf("Font loaded: %d glyphs, %d points, %d ascent, %d descent\n", currentFont.gCount, fontSize, currentFont.ascent, currentFont.descent);
}

void ST7789::setTextColor(uint16_t color, uint16_t bgColor)
{
    textcolor = color;
    textbgcolor = bgColor;
}

void ST7789::waitDMA()
{
    // nothing to do here
}

void ST7789::setRotation(uint8_t m)
{
    rotation = m % 4;
    uint8_t madctl = 0;
    switch (rotation)
    {
    case 0:
        madctl = MADCTL_MX | MADCTL_MY | MADCTL_RGB;
        width = 240;
        height = 320;
        break;
    case 1:
        madctl = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
        width = 320;
        height = 240;
        break;
    case 2:
        madctl = MADCTL_RGB;
        width = 240;
        height = 320;
        break;
    case 3:
        madctl = MADCTL_MX | MADCTL_MV | MADCTL_RGB;
        width = 320;
        height = 240;
        break;
    }
    sendCmd(ST7789_CMD_MADCTL);
    sendData(&madctl, 1);
}

Glyph ST7789::getGlyphData(uint32_t unicode)
{
    const uint8_t *fontPtr = currentFont.fontData + 24;
    const uint8_t *bitmapPtr = currentFont.fontData + 24 + currentFont.gCount * 28;

    for (uint32_t i = 0; i < currentFont.gCount; i++)
    {
        uint32_t glyphUnicode = readUInt32(fontPtr);
        uint32_t height = readUInt32(fontPtr + 4);
        uint32_t width = readUInt32(fontPtr + 8);
        int32_t gxAdvance = readInt32(fontPtr + 12);
        int32_t dY = readInt32(fontPtr + 16);
        int32_t dX = readInt32(fontPtr + 20);

        if (glyphUnicode == unicode)
        {
            Glyph glyph;
            glyph.unicode = glyphUnicode;
            glyph.width = width;
            glyph.height = height;
            glyph.gxAdvance = gxAdvance;
            glyph.dX = dX;
            glyph.dY = dY;
            glyph.bitmap = bitmapPtr;
            return glyph;
        }

        // Move to next glyph (28 bytes for metadata + bitmap size)
        fontPtr += 28;
        bitmapPtr += width * height;
    }

    // Return default glyph if not found
    Serial.printf("Glyph not found: %c\n", unicode);
    return getGlyphData(' ');
}

void ST7789::drawPixel(uint16_t color, int x, int y)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return;
    }
    setWindow(x, y, x, y);
    sendPixel(swapBytes(color));
}

void ST7789::drawGlyph(const Glyph &glyph, int x, int y)
{
    // Iterate over each pixel in the glyph's bitmap
    const uint8_t *bitmap = glyph.bitmap;
    uint16_t pixelBuffer[glyph.width * glyph.height] = {textbgcolor};
    for (int j = 0; j < glyph.height; j++)
    {
        for (int i = 0; i < glyph.width; i++)
        {
            // Get the alpha value for the current pixel (1 byte per pixel)
            uint8_t alpha = bitmap[j * glyph.width + i];
            if (alpha > 0)
            {
                // blend between the text color and the background color
                uint16_t fg = textcolor;
                uint16_t bg = textbgcolor;
                // extract the red, green and blue
                uint8_t fgRed = (fg >> 11) & 0x1F;
                uint8_t fgGreen = (fg >> 5) & 0x3F;
                uint8_t fgBlue = fg & 0x1F;

                uint8_t bgRed = (bg >> 11) & 0x1F;
                uint8_t bgGreen = (bg >> 5) & 0x3F;
                uint8_t bgBlue = bg & 0x1F;

                uint8_t red = ((fgRed * alpha) + (bgRed * (255 - alpha))) / 255;
                uint8_t green = ((fgGreen * alpha) + (bgGreen * (255 - alpha))) / 255;
                uint8_t blue = ((fgBlue * alpha) + (bgBlue * (255 - alpha))) / 255;

                uint16_t color = (red << 11) | (green << 5) | blue;
                pixelBuffer[i + j * glyph.width] = swapBytes(color);
            }
        }
    }
    setWindow(x + glyph.dX, y - glyph.dY, x + glyph.dX + glyph.width - 1, y + glyph.dY + glyph.height - 1);
    sendPixels(pixelBuffer, glyph.width * glyph.height);
}

void ST7789::drawString(const char *text, int16_t x, int16_t y)
{
    int cursorX = x;
    int cursorY = y;

    while (*text)
    {
        char c = *text++;

        // Get the glyph data for the character
        Glyph glyph = getGlyphData((uint32_t)c);

        // Draw the glyph bitmap at the current cursor position
        drawGlyph(glyph, cursorX, cursorY + currentFont.ascent);

        // Move the cursor to the right by the glyph's gxAdvance value
        cursorX += glyph.gxAdvance;
    }
}