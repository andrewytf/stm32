#ifndef _CMDDETAIL_H
#define _CMDDETAIL_H

#include "ch.h"

void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]);

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)
#endif