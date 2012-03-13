
/*
 *     Copyright (C) 2011-2012 Andreas Schönfelder
 *     https://github.com/escoand/socksswitch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "trace.h"

#ifdef WIN32
#include <windef.h>
#include <psapi.h>
#include <windows.h>
#endif

void
trace(const char *file, const int line, const char *function,
      enum TRACE_LEVEL level, const char *format, ...) {
    const char *tl[] = { "", "ERROR", "WARNING", "INFO", "VERBOSE" };
    FILE *output;
    char datestr[9], timestr[9], msg1[256], msg2[1025];
    va_list ap;

    /*  check if log needed */
    if (getenv("TRACE") == NULL || level > atoi(getenv("TRACE")))
	return;

    /*  get date and time */
#ifdef WIN32
    _strdate(datestr);
    _strtime(timestr);
#else
    time_t rawtime;
    time(&rawtime);
    strftime(datestr, 9, "%x", localtime(&rawtime));
    strftime(timestr, 9, "%X", localtime(&rawtime));
#endif

    /*  output message */
    sprintf(msg1, "[%s %s] ", datestr, timestr);
    if (file && line && function)
	sprintf(msg1, "%s%s:%d:%s: ", msg1, file, line, function);
    if (level != TRACE_LEVEL_NO)
	sprintf(msg1, "%s%s: ", msg1, tl[level]);
    va_start(ap, format);
    vsprintf(msg2, format, ap);
    va_end(ap);

    /*  log to file */
    if (getenv("LOGFILE") != NULL)
	output = fopen(getenv("LOGFILE"), "a");

    /*  log to stdout and stderr */
    else {
	if (level == TRACE_LEVEL_ERROR)
	    output = stderr;
	else
	    output = stdout;
    }
    fputs(strcat(msg1, msg2), output);

    /*  close log file */
    if (getenv("LOGFILE") != NULL)
	fclose(output);
}

void trace_append(enum TRACE_LEVEL level, const char *format, ...) {
    FILE *output;
    va_list ap;

    /*  check if log needed */
    if (getenv("TRACE") == NULL || level > atoi(getenv("TRACE")))
	return;

    /*  log to file */
    if (getenv("LOGFILE") != NULL)
	output = fopen(getenv("LOGFILE"), "a");

    /*  log to stdout and stderr */
    else {
	if (level == TRACE_LEVEL_ERROR)
	    output = stderr;
	else
	    output = stdout;
    }

    /*  output message */
    va_start(ap, format);
    vfprintf(output, format, ap);
    va_end(ap);

    /*  close log file */
    if (getenv("LOGFILE") != NULL)
	fclose(output);
}

void trace_dump(const void *data, int len) {
    int i, j;
    char msg1[1024 * 10 * 4] = "", msg2[16];
    FILE *output;

    /*  check if log needed */
    if (getenv("DUMP") == NULL)
	return;

    /*  output hexdump */
    for (i = 0; i < len; i = i + 16) {

	/* pos */
	sprintf(msg2, "%08x  ", i);
	strcpy(msg1 + strlen(msg1), msg2);

	/* hex */
	for (j = i; j < i + 16 && j < len; j++) {
	    sprintf(msg2, "%02x ", ((unsigned char *) data)[j]);
	    strcpy(msg1 + strlen(msg1), msg2);
	}

	/* spacer */
	for (; j < i + 16; j++)
	    strcpy(msg1 + strlen(msg1), "   ");
	strcpy(msg1 + strlen(msg1), " ");

	/* ascii */
	for (j = i; j < i + 16 && j < len; j++) {
	    if (((unsigned char *) data)[j] < 0x20
		|| ((unsigned char *) data)[j] > 0x7e)
		strcpy(msg1 + strlen(msg1), ".");
	    else {
		sprintf(msg2, "%c", ((unsigned char *) data)[j]);
		strcpy(msg1 + strlen(msg1), msg2);
	    }
	}

	strcpy(msg1 + strlen(msg1), "\n");
    }

    /*  log to file */
    if (getenv("LOGFILE") != NULL)
	output = fopen(getenv("LOGFILE"), "a");
    else
	output = stdout;

    fputs(msg1, output);

    /*  close log file */
    if (getenv("LOGFILE") != NULL)
	fclose(output);
}

void trace_memory() {
    FILE *output;

    /*  log to file */
    if (getenv("LOGFILE") != NULL)
	output = fopen(getenv("LOGFILE"), "a");
    else
	output = stdout;

#ifdef WIN32
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			   PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    if (NULL == hProcess)
	return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
	fprintf
	    (output, "pf:%u pm:%u cm:%u ppp:%u pp:%u pnpp:%u npp:%u\n",
	     (unsigned int) pmc.PageFaultCount,
	     (unsigned int) pmc.PeakWorkingSetSize,
	     (unsigned int) pmc.WorkingSetSize,
	     (unsigned int) pmc.QuotaPeakPagedPoolUsage,
	     (unsigned int) pmc.QuotaPagedPoolUsage,
	     (unsigned int) pmc.QuotaPeakNonPagedPoolUsage,
	     (unsigned int) pmc.QuotaNonPagedPoolUsage);
    }

    CloseHandle(hProcess);
#else
    fprintf(output, "\n");
#endif

    /*  close log file */
    if (getenv("LOGFILE") != NULL)
	fclose(output);
}
