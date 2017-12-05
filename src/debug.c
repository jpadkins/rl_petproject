///////////////////////////////////////////////////////////////////////////////
/// @file   debug.c
/// @brief  Provides an API for logging and halting execution if necessary
/// @author Jacob Adkins (jpadkins)
///////////////////////////////////////////////////////////////////////////////

#include "debug.h"

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
/// Defines
///////////////////////////////////////////////////////////////////////////////

#define MSGBUFF_SIZE 1024

///////////////////////////////////////////////////////////////////////////////
void debug_log(DebugLevel level, const char *file, const char *func, int line,
    const char *msg, ...)
{
    va_list ap;
    char msgbuff[MSGBUFF_SIZE];

    va_start(ap, msg);
    vsprintf(msgbuff, msg, ap);
    va_end(ap);

    switch (level) {
    case DEBUG_INFO:
        fprintf(stdout, "[INFO][%s][%s][%x]: %s\n", file, func, line, msgbuff);
        break;
    case DEBUG_WARN:
        fprintf(stderr, "[WARN][%s][%s][%x]: %s\n", file, func, line, msgbuff);
        break;
    case DEBUG_EXIT:
        fprintf(stderr, "[EXIT][%s][%s][%x]: %s\n", file, func, line, msgbuff);
        exit(EXIT_FAILURE);
        break;
    }
}
