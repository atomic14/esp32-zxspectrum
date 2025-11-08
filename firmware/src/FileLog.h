#pragma once

#include "Config.h"
#include "Files/Files.h"

#define LOG_BUF_SIZE 1024
#define MAX_BYTE_LEN 16

class FileLog 
{
private:
    IFiles *files;
    const char *filename = "z80.log";
    const bool active = false;

public:
    FileLog()
    {
        files = Config::getConfig().sdfiles;
    }

    void log(const char *format, ...);
    void logbytes(const char *start, const char *end, uint8_t *data, int data_length);
    void clear();
};