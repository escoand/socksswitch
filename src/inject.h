#include <windows.h>

BOOL inject(int pid, const char *dll);
void printModules(DWORD pid);
DWORD getProcId(const char *ProcName);
