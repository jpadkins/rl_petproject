///////////////////////////////////////////////////////////////////////////////
/// @file:		log.c
/// @author:	Jacob Adkins (jpadkins)
/// @brief:		Provides a simple C99 API for logging and halting execution if
///				necessary.
///////////////////////////////////////////////////////////////////////////////

#include "log.h"

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
void _log(enum _LogLevel level, const char *file, const char *func, int line,
	const char *msg, ...)
{
	va_list ap;
	char msgbuff[MSGBUFF_SIZE];

	va_start(ap, msg);
	vsprintf(msgbuff, msg, ap);
	va_end(ap);

	switch (level) {
	case LOG_INFO:
		fprintf(stdout, "[\33[34;1mINFO\33[0m][%s][%s][%d]: %s\n", file, func,
			line, msgbuff);
		break;
	case LOG_WARN:
		fprintf(stderr, "[\33[33;1mWARN\33[0m][%s][%s][%d]: %s\n", file, func,
			line, msgbuff);
		break;
	case LOG_EXIT:
		fprintf(stderr, "[\33[31;1mEXIT\33[0m][%s][%s][%d]: %s\n", file, func,
			line, msgbuff);
		exit(EXIT_FAILURE);
		break;
	}
}
