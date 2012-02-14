
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


#include <windows.h>
#include <io.h>
#include <string.h>
#include <tlhelp32.h>
#include "inject.h"
#include "trace.h"

int socksswitch_inject(char *path, const char *dll) {
    char *filename;
    PROCESSENTRY32 pe;
    MODULEENTRY32 me;
    HANDLE snapshot_proc, snapshow_mod, proc;
    FARPROC load;
    LPVOID addr;
    BOOL inj;

    DEBUG_ENTER;

    /* try to read file */
    if (access(dll, 04) != 0) {
	TRACE_WARNING("unable to read %s\n", dll);
	DEBUG_LEAVE;
	return 0;
    }
    //path = realpath(path, NULL);
    filename = strrchr(path, '\\') + 1;

    DEBUG;

    /* snapshot process */
    snapshot_proc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_proc == INVALID_HANDLE_VALUE) {
	TRACE_WARNING
	    ("failure on creating toolhelp snapshot(err:%d)\n ",
	     GetLastError());
	DEBUG_LEAVE;
	return 0;
    }

    DEBUG;

    pe.dwSize = sizeof(pe);
    me.dwSize = sizeof(me);

    DEBUG;

    /* run processes */
    while (Process32Next(snapshot_proc, &pe)) {
	inj = FALSE;

	/* check exe name */
	if (strcasecmp(pe.szExeFile, filename) != 0)
	    continue;

	/* snapshot modules */
	snapshow_mod = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
						pe.th32ProcessID);
	if (snapshow_mod == INVALID_HANDLE_VALUE) {
	    TRACE_WARNING
		("failure on creating toolhelp snapshot(err:%d)\n",
		 GetLastError());
	    DEBUG_LEAVE;
	    return 0;
	}

	DEBUG;

	/* run modules */
	while (Module32Next(snapshow_mod, &me)) {

	    /* check exe path */
	    if (strcasecmp(me.szExePath, path) == 0)
		inj = TRUE;

	    /* check if allready injected */
	    else if (strcasecmp(me.szExePath, dll) == 0) {
		inj = FALSE;
		break;
	    }
	}

	DEBUG;

	/* inject */
	if (inj) {

	    /* address of load library */
	    if ((load =
		 GetProcAddress(LoadLibrary("kernel32.dll"),
				"LoadLibraryA")) == NULL) {
		DEBUG_LEAVE;
		return 0;
	    }

	    DEBUG;

	    /* open process */
	    if ((proc =
		 OpenProcess(PROCESS_ALL_ACCESS, 0,
			     pe.th32ProcessID)) == NULL)
		return 0;

	    DEBUG;

	    /* access */
	    if ((addr =
		 VirtualAllocEx(proc, NULL, strlen(dll),
				MEM_COMMIT | MEM_RESERVE,
				PAGE_EXECUTE_READWRITE)) == 0) {
		DEBUG_LEAVE;
		return 0;
	    }

	    DEBUG;

	    /* write dll path */
	    if (WriteProcessMemory
		(proc, addr, (LPVOID) dll, strlen(dll), NULL) == 0) {
		DEBUG_LEAVE;
		return 0;
	    }

	    DEBUG;

	    /* load library */
	    if (CreateRemoteThread(proc, 0, 0,
				   (LPTHREAD_START_ROUTINE) load, addr,
				   0, 0) == NULL) {
		DEBUG_LEAVE;
		return 0;
	    }

	    TRACE_INFO("injected into \"%s\" (pid:%i)\n", path,
		       pe.th32ProcessID);
	}
    }

    DEBUG_LEAVE;

    return 1;
}
