#ifndef WII_I2C_H_FILE
#define WII_I2C_H_FILE

#define WII_I2C_ENABLE_MULTI_CORE 1  // 0=disable, 1=enable

#define WII_I2C_IDENT_NONE     0
#define WII_I2C_IDENT_NUNCHUK  0xa4200000
#define WII_I2C_IDENT_CLASSIC  0xa4200101

#ifdef __cplusplus
extern "C" {
#endif

struct wii_i2c_nunchuk_state {
  // accelerometer
  short int acc_x;
  short int acc_y;
  short int acc_z;

  // analog stick:
  signed char x;
  signed char y;

  // buttons:
  char c;
  char z;
};

struct wii_i2c_classic_state {
  // analog sticks:
  signed char lx;
  signed char ly;
  signed char rx;
  signed char ry;

  // triggers (a_ is the analog part, d_ is the click bit):
  unsigned char a_lt;
  unsigned char a_rt;
  char d_lt;
  char d_rt;

  // d-pad:
  char up;
  char down;
  char left;
  char right;

  // buttons:
  char a;
  char b;
  char x;
  char y;

  // bumpers:
  char zr;
  char zl;

  // face buttons:
  char home;
  char plus;
  char minus;
};

// common functions (single-core and multi-core mode):
int wii_i2c_init(int i2c_port_num, int sda_pin, int scl_pin);
const unsigned char *wii_i2c_read_ident(void);
unsigned int wii_i2c_decode_ident(const unsigned char *ident);
void wii_i2c_decode_nunchuk(const unsigned char *data, struct wii_i2c_nunchuk_state *state);
void wii_i2c_decode_classic(const unsigned char *data, struct wii_i2c_classic_state *state);

// for use in single-core mode:
int wii_i2c_request_state(void);
const unsigned char *wii_i2c_read_state(void);

#if WII_I2C_ENABLE_MULTI_CORE
// for use in multi-core mode:
int wii_i2c_start_read_task(int cpu_num, int delay);
const unsigned char *wii_i2c_read_data_from_task(void);
#endif /* WII_I2C_ENABLE_MULTI_CORE */
  
#ifdef __cplusplus
}
#endif

#endif /* WII_I2C_H_FILE */
