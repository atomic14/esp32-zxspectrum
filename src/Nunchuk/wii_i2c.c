#include <driver/i2c.h>
#include <string.h>

#include "wii_i2c.h"

#define WII_I2C_ADDR  0x52

static const uint8_t data_init1[]     = { 0xf0, 0x55 };
static const uint8_t data_init2[]     = { 0xfb, 0x00 };
static const uint8_t data_req_ident[] = { 0xfa };
static const uint8_t data_req_data[]  = { 0x00 };

static i2c_port_t wii_i2c_port_num;
static uint8_t read_data[6];

static TaskHandle_t read_state_task_handle;
static int read_state_task_delay;
static SemaphoreHandle_t read_mutex;
static uint8_t shared_read_data[6];
static volatile uint8_t shared_copy_data[6];
static volatile uint32_t shared_copy_ready;

static esp_err_t wii_i2c_setup_i2c(i2c_port_t i2c_port_num, int sda_pin, int scl_pin)
{
  wii_i2c_port_num = i2c_port_num;
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num    = sda_pin,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num    = scl_pin,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000,  // 100KHz
  };
  i2c_param_config(wii_i2c_port_num, &conf);
  return i2c_driver_install(wii_i2c_port_num, conf.mode, 0, 0, 0);
}

static esp_err_t wii_i2c_write(const uint8_t *data, size_t len)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, (uint8_t *)data, len, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(wii_i2c_port_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t wii_i2c_read(uint8_t *data, size_t len)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(wii_i2c_port_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

int wii_i2c_init(int i2c_port_num, int sda_pin, int scl_pin)
{
  if (wii_i2c_setup_i2c(i2c_port_num, sda_pin, scl_pin) != ESP_OK) return 1;
  if (wii_i2c_write(data_init1, sizeof(data_init1)) != ESP_OK) return 1;
  if (wii_i2c_write(data_init2, sizeof(data_init2)) != ESP_OK) return 1;
  return 0;
}

const unsigned char *wii_i2c_read_ident(void)
{
  if (wii_i2c_write(data_req_ident, sizeof(data_req_ident)) != ESP_OK) return NULL;
  if (wii_i2c_read(read_data, sizeof(read_data)) != ESP_OK) return NULL;
  return (const unsigned char *) read_data;
}

int wii_i2c_request_state(void)
{
  if (wii_i2c_write(data_req_data, sizeof(data_req_data)) != ESP_OK) return 1;
  return 0;
}

const unsigned char *wii_i2c_read_state(void)
{
  if (wii_i2c_read(read_data, sizeof(read_data)) != ESP_OK) return NULL;
  return (const unsigned char *) read_data;
}

unsigned int wii_i2c_decode_ident(const unsigned char *ident)
{
  if (! ident) return WII_I2C_IDENT_NONE;
  return (((uint32_t)ident[5] <<  0) |
          ((uint32_t)ident[4] <<  8) |
          ((uint32_t)ident[3] << 16) |
          ((uint32_t)ident[2] << 24));
}

void wii_i2c_decode_nunchuk(const unsigned char *data, struct wii_i2c_nunchuk_state *state)
{
  if (! data) {
    memset(state, 0, sizeof(*state));
    return;
  }

  state->x = data[0] - (1<<7);
  state->y = data[1] - (1<<7);
  state->acc_x = ((data[2] << 2) | ((data[5] & 0x0c) >> 2)) - (1<<9);
  state->acc_y = ((data[3] << 2) | ((data[5] & 0x30) >> 4)) - (1<<9);
  state->acc_z = ((data[4] << 2) | ((data[5] & 0xc0) >> 6)) - (1<<9);
  state->c = (data[5] & 0x02) ? 0 : 1;
  state->z = (data[5] & 0x01) ? 0 : 1;
}

void wii_i2c_decode_classic(const unsigned char *data, struct wii_i2c_classic_state *state)
{
  if (! data) {
    memset(state, 0, sizeof(*state));
    return;
  }
  
  state->lx = (data[0] & 0x3f) - (1<<5);
  state->ly = (data[1] & 0x3f) - (1<<5);
  state->rx = (((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) - (1<<4);
  state->ry = (data[2] & 0x1f) - (1<<4);
  
  state->a_lt = ((data[2] & 0x60) >> 2) | ((data[3] & 0xe0) >> 5);
  state->a_rt = data[3] & 0x1f;
  state->d_lt = (data[4] & 0x20) ? 0 : 1;
  state->d_rt = (data[4] & 0x02) ? 0 : 1;
  
  state->left  = (data[5] & 0x02) ? 0 : 1;
  state->right = (data[4] & 0x80) ? 0 : 1;
  state->up    = (data[5] & 0x01) ? 0 : 1;
  state->down  = (data[4] & 0x40) ? 0 : 1;
  state->a     = (data[5] & 0x10) ? 0 : 1;
  state->b     = (data[5] & 0x40) ? 0 : 1;
  state->x     = (data[5] & 0x08) ? 0 : 1;
  state->y     = (data[5] & 0x20) ? 0 : 1;
  state->plus  = (data[4] & 0x04) ? 0 : 1;
  state->minus = (data[4] & 0x10) ? 0 : 1;
  state->home  = (data[4] & 0x08) ? 0 : 1;
  state->zl    = (data[5] & 0x80) ? 0 : 1;
  state->zr    = (data[5] & 0x04) ? 0 : 1;
}

#if WII_I2C_ENABLE_MULTI_CORE

static void read_state_task_func(void *params)
{
  while (true) {
    wii_i2c_request_state();
    vTaskDelay(read_state_task_delay / portTICK_PERIOD_MS);
    if (wii_i2c_read(shared_read_data, sizeof(shared_read_data)) == ESP_OK) {
      xSemaphoreTake(read_mutex, portMAX_DELAY);
      memcpy((unsigned char *) shared_copy_data, shared_read_data, sizeof(shared_copy_data));
      shared_copy_ready = 1;
      xSemaphoreGive(read_mutex);
    }
  }
}

int wii_i2c_start_read_task(int cpu_num, int delay)
{
  read_state_task_delay = delay;
  read_mutex = xSemaphoreCreateMutex();
  if (! read_mutex) {
    return 1;
  }
  
  BaseType_t ret = xTaskCreatePinnedToCore(read_state_task_func, "wiiI2CTask", 1024, NULL, 0, &read_state_task_handle, cpu_num);
  if (ret != pdPASS) {
    vSemaphoreDelete(read_mutex);
    return 1;
  }
  return 0;
}

const unsigned char *wii_i2c_read_data_from_task()
{
  if (! shared_copy_ready) return NULL;
  
  xSemaphoreTake(read_mutex, portMAX_DELAY);
  memcpy(read_data, (unsigned char *) shared_copy_data, sizeof(read_data));
  shared_copy_ready = 0;
  xSemaphoreGive(read_mutex);
  
  return (const unsigned char *) read_data;
}

#endif /* WII_I2C_ENABLE_MULTI_CORE */
