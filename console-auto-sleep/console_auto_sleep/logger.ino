#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

const int BUF_SIZE = 1024;
char buf[BUF_SIZE];

const int FMT_BUF_SIZE = 1024;
char fmtBuf[FMT_BUF_SIZE];

void ActivityLogger::log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    snprintf(fmtBuf, FMT_BUF_SIZE, "[%s] %s", this->module, fmt);
    vsnprintf(buf, BUF_SIZE, fmtBuf, args);
    va_end(args);
    Serial.println(buf);
}
