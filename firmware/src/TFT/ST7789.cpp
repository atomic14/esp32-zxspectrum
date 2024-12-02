#include "Arduino.h"
#include "Serial.h"
#include "ST7789.h"
#include "esp_log.h"

// ST7789 specific commands used in init
#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_RDDPM 0x0A      // Read display power mode
#define ST7789_RDD_MADCTL 0x0B // Read display MADCTL
#define ST7789_RDD_COLMOD 0x0C // Read display pixel format
#define ST7789_RDDIM 0x0D      // Read display image mode
#define ST7789_RDDSM 0x0E      // Read display signal mode
#define ST7789_RDDSR 0x0F      // Read display self-diagnostic result (ST7789V)

#define ST7789_SLPIN 0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_GAMSET 0x26 // Gamma set
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RGBSET 0x2D // Color setting for 4096, 64K and 262K colors
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_VSCRDEF 0x33  // Vertical scrolling definition (ST7789V)
#define ST7789_TEOFF 0x34    // Tearing effect line off
#define ST7789_TEON 0x35     // Tearing effect line on
#define ST7789_MADCTL 0x36   // Memory data access control
#define ST7789_VSCRSADD 0x37 // Vertical screoll address
#define ST7789_IDMOFF 0x38   // Idle mode off
#define ST7789_IDMON 0x39    // Idle mode on
#define ST7789_RAMWRC 0x3C   // Memory write continue (ST7789V)
#define ST7789_RAMRDC 0x3E   // Memory read continue (ST7789V)
#define ST7789_COLMOD 0x3A

#define ST7789_RAMCTRL 0xB0   // RAM control
#define ST7789_RGBCTRL 0xB1   // RGB control
#define ST7789_PORCTRL 0xB2   // Porch control
#define ST7789_FRCTRL1 0xB3   // Frame rate control
#define ST7789_PARCTRL 0xB5   // Partial mode control
#define ST7789_GCTRL 0xB7     // Gate control
#define ST7789_GTADJ 0xB8     // Gate on timing adjustment
#define ST7789_DGMEN 0xBA     // Digital gamma enable
#define ST7789_VCOMS 0xBB     // VCOMS setting
#define ST7789_LCMCTRL 0xC0   // LCM control
#define ST7789_IDSET 0xC1     // ID setting
#define ST7789_VDVVRHEN 0xC2  // VDV and VRH command enable
#define ST7789_VRHS 0xC3      // VRH set
#define ST7789_VDVSET 0xC4    // VDV setting
#define ST7789_VCMOFSET 0xC5  // VCOMS offset set
#define ST7789_FRCTR2 0xC6    // FR Control 2
#define ST7789_CABCCTRL 0xC7  // CABC control
#define ST7789_REGSEL1 0xC8   // Register value section 1
#define ST7789_REGSEL2 0xCA   // Register value section 2
#define ST7789_PWMFRSEL 0xCC  // PWM frequency selection
#define ST7789_PWCTRL1 0xD0   // Power control 1
#define ST7789_VAPVANEN 0xD2  // Enable VAP/VAN signal output
#define ST7789_CMD2EN 0xDF    // Command 2 enable
#define ST7789_PVGAMCTRL 0xE0 // Positive voltage gamma control
#define ST7789_NVGAMCTRL 0xE1 // Negative voltage gamma control
#define ST7789_DGMLUTR 0xE2   // Digital gamma look-up table for red
#define ST7789_DGMLUTB 0xE3   // Digital gamma look-up table for blue
#define ST7789_GATECTRL 0xE4  // Gate control
#define ST7789_SPI2EN 0xE7    // SPI2 enable
#define ST7789_PWCTRL2 0xE8   // Power control 2
#define ST7789_EQCTRL 0xE9    // Equalize time control
#define ST7789_PROMCTRL 0xEC  // Program control
#define ST7789_PROMEN 0xFA    // Program mode enable
#define ST7789_NVMSET 0xFC    // NVM setting
#define ST7789_PROMACT 0xFE   // Program action

ST7789::ST7789(gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height)
    : TFTDisplay(cs, dc, rst, bl, width, height)
{
    // Reset the display
    digitalWrite(rst, LOW);
    // gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(rst, HIGH);
    // gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    SEND_CMD_DATA(ST7789_SLPOUT); // Sleep out
    delay(120);
    SEND_CMD_DATA(ST7789_NORON); // Normal display mode on
    SEND_CMD_DATA(ST7789_MADCTL, TFT_MAD_BGR);
    SEND_CMD_DATA(ST7789_RAMCTRL, 0x00, 0xE0); // 5 to 6-bit conversion: r0 = r5, b0 = b5
    SEND_CMD_DATA(ST7789_COLMOD, 0x55);
    delay(10);
    //--------------------------------ST7789V Frame rate setting----------------------------------//
    SEND_CMD_DATA(ST7789_PORCTRL, 0x0B, 0x0B, 0x00, 0x33, 0x33);
    SEND_CMD_DATA(ST7789_GCTRL, 0x75); // Voltages: VGH / VGL
    //---------------------------------ST7789V Power setting--------------------------------------//
    SEND_CMD_DATA(ST7789_VCOMS, 0x28); // JLX240 display datasheet
    SEND_CMD_DATA(ST7789_LCMCTRL, 0x2C);
    SEND_CMD_DATA(ST7789_VDVVRHEN, 0x01);
    SEND_CMD_DATA(ST7789_VRHS, 0x1F); // Voltage VRHS
    SEND_CMD_DATA(ST7789_FRCTR2, 0x13);
    SEND_CMD_DATA(ST7789_PWCTRL1, 0xA7);
    SEND_CMD_DATA(ST7789_PWCTRL1, 0xA4, 0xA1);
    SEND_CMD_DATA(0xD6, 0xA1);
    //--------------------------------ST7789V gamma setting---------------------------------------//
    SEND_CMD_DATA(ST7789_PVGAMCTRL, 0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32);
    SEND_CMD_DATA(ST7789_NVGAMCTRL, 0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37);
    SEND_CMD_DATA(ST7789_INVON); // Display on
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    dmaWait();
    delay(120);
    SEND_CMD_DATA(ST7789_DISPON); // Display on
    delay(120);
    dmaWait();
    SEND_CMD_DATA(TFT_MADCTL, TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_RGB);

    Serial.println("ST7789 initialized");
}

ST7789::~ST7789()
{
}
