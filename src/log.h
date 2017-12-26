///////////////////////////////////////////////////////////////////////////////
/// @file:		log.h
/// @author:	Jacob Adkins (jpadkins)
/// @brief:		Provides a simple C99 API for logging and halting execution if
///				necessary.
///////////////////////////////////////////////////////////////////////////////

#ifndef LOG_H
#define LOG_H

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>

///////////////////////////////////////////////////////////////////////////////
/// @brief	Describes the severity of the message
///
/// LOG_INFO:	Logs the message
/// LOG_WARN:	Logs the message - denotes a runtime problem
/// LOG_EXIT:	Logs the message - denotes a programmer error, calls exit(1)
///////////////////////////////////////////////////////////////////////////////
enum _LogLevel {LOG_INFO, LOG_WARN, LOG_EXIT};

///////////////////////////////////////////////////////////////////////////////
/// @brief	Used internally by log_* macros
///
/// @param	level	Level of severity
/// @param	file	Name of the current file
/// @param	func	Name of the current function
/// @param	line	Current line number
/// @param	msg		Format string
/// @param	...		Format arguments
///////////////////////////////////////////////////////////////////////////////
void _log(enum _LogLevel level, const char *file, const char *func, int line,
	const char *msg, ...);

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a message
///
/// @param	msg	Message to log
///////////////////////////////////////////////////////////////////////////////
#define log_info(msg) _log(LOG_INFO,__FILE__,__func__,__LINE__,"%s",msg)

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a message that denotes a runtime error
///
/// @param	msg	Message to log
///////////////////////////////////////////////////////////////////////////////
#define log_warn(msg) _log(LOG_WARN,__FILE__,__func__,__LINE__,"%s",msg)

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a message that denotes a programmer error
///
/// The program exits after logging.
///
/// @param	msg	Message to log
///////////////////////////////////////////////////////////////////////////////
#define log_exit(msg) _log(LOG_EXIT,__FILE__,__func__,__LINE__,"%s",msg)

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a formatted message
///
/// @param	fmt	Format string
/// @param	...	Format arguments
///////////////////////////////////////////////////////////////////////////////
#define logfmt_info(fmt,...) _log(LOG_INFO,__FILE__,__func__,__LINE__,fmt,\
	__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a formatted message that denotes a runtime error
///
/// @param	fmt	Format string
/// @param	...	Format arguments
///////////////////////////////////////////////////////////////////////////////
#define logfmt_warn(fmt,...) _log(LOG_WARN,__FILE__,__func__,__LINE__,fmt,\
	__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
/// @brief	Logs a formatted message that denotes a programmer error
///
/// The program exits after logging.
///
/// @param	fmt	Format string
/// @param	...	Format arguments
///////////////////////////////////////////////////////////////////////////////
#define logfmt_exit(fmt,...) _log(LOG_EXIT,__FILE__,__func__,__LINE__,fmt,\
	__VA_ARGS__)

#endif
