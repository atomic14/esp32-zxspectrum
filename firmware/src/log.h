#pragma once

#include "Serial.h"

extern unsigned int master_log_level;

#define LOG_E(format, ...) if (master_log_level >= ESP_LOG_ERROR)   Serial.printf("E:" format "\n" __VA_OPT__(,) __VA_ARGS__)
#define LOG_W(format, ...) if (master_log_level >= ESP_LOG_WARN)    Serial.printf("W:" format "\n" __VA_OPT__(,) __VA_ARGS__)
#define LOG_I(format, ...) if (master_log_level >= ESP_LOG_INFO)    Serial.printf("I:" format "\n" __VA_OPT__(,) __VA_ARGS__)
#define LOG_D(format, ...) if (master_log_level >= ESP_LOG_DEBUG)   Serial.printf("D:" format "\n" __VA_OPT__(,) __VA_ARGS__)
#define LOG_V(format, ...) if (master_log_level >= ESP_LOG_VERBOSE) Serial.printf("V:" format "\n" __VA_OPT__(,) __VA_ARGS__)
