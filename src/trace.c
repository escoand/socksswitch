/*
 * trace.h
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include "trace.h"


void trace(const char *file, const int line, const char *function,
	   enum TRACE_LEVEL level, const char *format, ...) {
    const char *tl[] = { "", "ERROR", "WARNING", "INFO", "VERBOSE" };
    FILE *output;
    char datestr[9];
    char timestr[9];
    va_list ap;

    /*  check if log needed */
    if (level > atoi(getenv("TRACE")))
	return;

    /*  get date and time */
#ifdef _WIN32
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
    if (level > atoi(getenv("TRACE")))
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

void trace_dump(const char *data, int len) {
    int i, j;
    FILE *output;

    /*  check if log needed */
    if (atoi(getenv("DUMP")) < 1)
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
	    fprintf(output, "%02x ", (unsigned char) data[j]);

	/* spacer */
	for (; j < i + 16; j++)
	    fprintf(output, "   ");
	fprintf(output, " ");

	/* ascii */
	for (j = i; j < i + 16 && j < len; j++) {
	    if (data[j] < 32)
		fprintf(output, ".");
	    else
		fprintf(output, "%c", (unsigned char) data[j]);
	}

	fprintf(output, EOL);
    }
    /*if ((len * 3) % 80)
       fprintf(output, EOL); */

    /*  close log file */
    if (getenv("LOGFILE") != NULL)
	fclose(output);
}

void trace_memory() {
#ifdef _WIN32
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			   PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    if (NULL == hProcess)
	return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
	printf
	    ("pf:%u pm:%u cm:%u ppp:%u pp:%u pnpp:%u npp:%u\n",
	     (unsigned int) pmc.PageFaultCount,
	     (unsigned int) pmc.PeakWorkingSetSize,
	     (unsigned int) pmc.WorkingSetSize,
	     (unsigned int) pmc.QuotaPeakPagedPoolUsage,
	     (unsigned int) pmc.QuotaPagedPoolUsage,
	     (unsigned int) pmc.QuotaPeakNonPagedPoolUsage,
	     (unsigned int) pmc.QuotaNonPagedPoolUsage);
    }

    CloseHandle(hProcess);
#endif
}
