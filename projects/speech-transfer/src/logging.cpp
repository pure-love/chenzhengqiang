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


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include "logging.h"
#include <pthread.h>

/*******************************************************************************
 * Prototypes                                                                   *
 *******************************************************************************/

static const char *LOG_LEVEL_DESC[]={"_ERROR_","_INFO_","_INFOV_","_DEBUG_","_DEBUGV_"};

static void log_impl(int level, const char *module, const char * format, va_list valist);

/*******************************************************************************
 * Global variables                                                             *
 *******************************************************************************/

/**
 * Current verbosity level.
 * Used to determine when to send text from a printlog call to the log output.
 */
static int verbosity = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *logFP = NULL;
static char INITIAL_LOG_FILE[99];
static char CURRENT_LOG_FILE[99];

//static int KEEP_DAYS = 7;
static int64_t FILE_SIZE_LIMIT = 1024*1024*1024;

/*******************************************************************************
 * Global functions                                                             *
 *******************************************************************************/
int logging_init(const char *filepath, int logLevel)
{
    logFP = fopen( filepath, "w" );
    if (logFP == NULL)
    {
        return -1;
    }
    
    strncpy( INITIAL_LOG_FILE,filepath, sizeof( INITIAL_LOG_FILE ) );
    strncpy( CURRENT_LOG_FILE,filepath,sizeof(CURRENT_LOG_FILE) );

    /* Turn off buffering */
    setbuf( logFP, NULL );
    verbosity = logLevel;
    return 0;
}


void logging_deinit(void)
{
    if (logFP != NULL)
       fclose(logFP);
}


int64_t logfile_size()
{        
    struct stat statbuff; 
    if( stat(CURRENT_LOG_FILE, &statbuff) < 0 ){    
        return -1;    
    }
    return statbuff.st_size;
}

int log_reopen( void )
{
    char new_file[255];
    char timeBuffer[24];/* "YYYY-MM-DD HH:MM:SS" */
    time_t curtime;
    struct tm *loctime;

    /* Get the current time */
    curtime = time ( NULL );
    /* Convert it to local time representation. */
    loctime = localtime( &curtime );
    /* Print it out in a nice format. */
    strftime ( timeBuffer, sizeof(timeBuffer) , "%Y%m%d-%H%M%S", loctime );
    snprintf( new_file, sizeof(new_file) , "%s.%s", INITIAL_LOG_FILE , timeBuffer );
    memset( CURRENT_LOG_FILE,0, sizeof(CURRENT_LOG_FILE) );
    strncpy( CURRENT_LOG_FILE, new_file , sizeof(CURRENT_LOG_FILE) );
    rename( INITIAL_LOG_FILE, CURRENT_LOG_FILE );
    fclose( logFP );
    
    logFP = fopen( INITIAL_LOG_FILE, "w" );
    
    if ( logFP == NULL )
    {
        return -1;
    }
    
    /* Turn off buffering */
    setbuf( logFP, NULL );
    return 0;
}


void loglevel_set(int level)
{
    verbosity = level;
}

int loglevel_get(void)
{
    return verbosity;
}

void loglevel_inc(void)
{
    verbosity++;
}

void loglevel_dec(void)
{
    verbosity--;
}

unsigned int loglevel_is_enabled(int level)
{
    return (level <= verbosity);
}

void log_module(int level, const char *module, const char *format, ...)
{
    va_list valist;
    va_start(valist, format);

    if ( level <= verbosity )
    {
        log_impl(level, module, format, valist);
    }
    va_end(valist);
}
/*******************************************************************************
 * Local Functions                                                              *
 *******************************************************************************/

static void log_impl(int level, const char *module, const char * format, va_list valist)
{
    char timeBuffer[24]; /* "YYYY-MM-DD HH:MM:SS" */
    time_t curtime;
    struct tm *loctime;
    pthread_mutex_lock(&mutex);
    if( logfile_size() >= FILE_SIZE_LIMIT )
    {
         fclose(logFP);
         logFP = NULL;
         if( log_reopen() == -1 )
         {
             pthread_mutex_unlock(&mutex);
             return;
         }
    }
    if (!logFP){
        return;
    }
    /* Get the current time. */
    curtime = time (NULL);
    /* Convert it to local time representation. */
    loctime = localtime(&curtime);
    /* Print it out in a nice format. */
    strftime (timeBuffer, sizeof(timeBuffer), "%F %T : ", loctime);
    
    fprintf( logFP, "%s %-20s : %-8s : ", timeBuffer, module ? module:"<Unknown>", LOG_LEVEL_DESC[level] );

    vfprintf( logFP, format, valist );

    if( strchr( format, '\n' ) == NULL)
    {
        fprintf( logFP, "\n");
    }

    if ( ( level == LOG_ERROR ) && ( errno != 0) )
    {
        fprintf( logFP, "%s %-20s : %-8s : errno = %d (%s)\n", timeBuffer,
                module ? module:"<Unknown>", LOG_LEVEL_DESC[level], errno, strerror(errno));
    }
    pthread_mutex_unlock(&mutex);
}
