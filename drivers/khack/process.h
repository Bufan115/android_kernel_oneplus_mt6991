#ifndef _PROCESS_H
#define _PROCESS_H

#include <linux/types.h>

uintptr_t get_module_base(pid_t pid, const char* name);

#endif
