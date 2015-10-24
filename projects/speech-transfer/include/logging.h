/*
   Copyright (C) 2010 - xiecc
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
   */

/*
#@Modified-Date:2015/3/31
#@Who-Modified:chenzhengqiang
#@Desc:just change the function name for the sake of being consistent
*/

#ifndef _LOGGING_H
#define _LOGGING_H
#include <stdarg.h>
#include <stdint.h>
#include <sys/stat.h>
/**
 * Error logging level, always printed used for fatal error messages.
 */


#define LOG_ERROR    0

/**
 * Information logging level, used for warnings and other information.
 */
#define LOG_INFO      1

/**
 * Verbose Information logging level, less important than information level but
 * not quite debugging.
 */
#define LOG_INFOV     2 /* Verbose information */

/**
 * Debug Logging Level, useful debugging information.
 */
#define LOG_DEBUG    3

/**
 * Verbose Debugging Level, less useful debugging information.
 */
#define LOG_DEBUGV  4 /* Verbose debugging info */

/**
 * Diarrhee level, lots and lots of pointless text.
 */
#define LOG_DIARRHEA 5


#define MACRO_TO_STRING(MACRO) #MACRO
#define LINE_TO_STRING(LINE)  MACRO_TO_STRING(LINE)
#define LOG_LOCATION "  @File:"__FILE__"  @Line:"LINE_TO_STRING(__LINE__)
/**
 * @internal
 * Initialises logging, by first attempting to create the log file in /var/log,
 * then if unsuccessful in ~/

 * @param filename Name of the log file to create.
 * @param logLevel The initial logging/verbosity level.
 * @return 0 on success.
 */
int logging_init(const char *filename, int logLevel);

/**
 * @internal
 * Deinitialise logging.
 */
void logging_deinit(void);

/**
 * Set the current logging level.
 * @param level The new level to set.
 */
void loglevel_set(int level);

/**
 * Retrieves the current logging level.
 * @return The current logging level.
 */
int loglevel_get(void);

/**
 * Increase the logging level by 1.
 */
void loglevel_inc(void);

/**
 * Decrease the logging level by 1.
 */
void loglevel_dec(void);

/**
 * Determine if the specified logging level is enabled.
 * @param level The level to check.
 * @return TRUE if the level is enabled, FALSE otherwise.
 */
unsigned int loglevel_is_enabled(int level);

/**
 * Write the text describe by format to the log output, if the current verbosity
 * level is greater or equal to level.
 * @param level The level at which to output this text.
 * @param module The module that is doing the logging.
 * @param format String in printf format to output.
 */
extern void log_module(int level, const char *module, const char *format, ...);
#endif
