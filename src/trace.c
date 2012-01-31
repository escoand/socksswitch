
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
#include <stdio.h>
#include "trace.h"

#ifdef WIN32
#include <windows.h>
#include <psapi.h>
#endif


void
trace(const char *file, const int line, const char *function,
      enum TRACE_LEVEL level, const char *format, ...) {
    const char *tl[] = { "", "ERROR", "WARNING", "INFO", "VERBOSE" };
    FILE *output;
    char datestr[9];
    char timestr[9];
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
    fprintf(output, "[%s %s] ", datestr, timestr);
    if (file && line && function)
	fprintf(output, "%s:%d:%s: ", file, line, function);
    if (level != TRACE_LEVEL_NO)
	fprintf(output, "%s: ", tl[level]);
    va_start(ap, format);
    vfprintf(output, format, ap);
    va_end(ap);

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
    FILE *output;

    /*  check if log needed */
    if (getenv("DUMP") == NULL)
	return;

    /*  log to file */
    if (getenv("LOGFILE") != NULL)
	output = fopen(getenv("LOGFILE"), "a");
    else
	output = stdout;

    /*  output hexdump */
    for (i = 0; i < len; i = i + 16) {

	/* pos */
	fprintf(output, "%08x  ", i);

	/* hex */
	for (j = i; j < i + 16 && j < len; j++)
	    fprintf(output, "%02x ", ((unsigned char *) data)[j]);

	/* spacer */
	for (; j < i + 16; j++)
	    fprintf(output, "   ");
	fprintf(output, " ");

	/* ascii */
	for (j = i; j < i + 16 && j < len; j++) {
	    if (((unsigned char *) data)[j] < 0x20
		|| ((unsigned char *) data)[j] > 0x7e)
		fprintf(output, ".");
	    else
		fprintf(output, "%c", ((unsigned char *) data)[j]);
	}

	fprintf(output, EOL);
    }

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
