#include <cstdarg>
#include "FileLog.h"

void FileLog::log(const char *format, ...)
{
    if (!active) return;

    FILE *f = files->open(filename, "at");
    char buf[LOG_BUF_SIZE];
    int len = snprintf(buf, LOG_BUF_SIZE, "\n%d: ", millis());
    fwrite(buf, 1, len, f);

    va_list args;
    va_start(args, format);
    len = vsnprintf(buf, LOG_BUF_SIZE, format, args);
    va_end(args);

    fwrite(buf, 1, len, f);
    buf[0] = '\n';
    fwrite(buf, 1, 1, f);
    fclose(f);
}

void FileLog::logbytes(const char *start, const char *end, uint8_t *data, int data_length)
{
    if (!active) return;

    char buf[8];
    const char sep[2] = {',', ' '};

    if (data_length > MAX_BYTE_LEN){
        data_length = MAX_BYTE_LEN;
    }

    FILE *f = files->open(filename, "at");
    if (start){
        fwrite(start, 1, strlen(start), f);
    }
    for (int i=0; i<data_length; i++){
        if (i > 0){
            fwrite(sep, 1, sizeof(sep), f);
        }
        int len = snprintf(buf, sizeof(buf), "%02X", data[i]);
        fwrite(buf, 1, len, f);
    }
    if (end){
        fwrite(end, 1, strlen(end), f);
    }
    buf[0] = '\n';
    fwrite(buf, 1, 1, f);
    fclose(f);
}

void FileLog::clear()
{
    FILE *f = files->open(filename, "w");
    fclose(f);
}