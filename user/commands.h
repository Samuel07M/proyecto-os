#ifndef COMMANDS_H
#define COMMANDS_H

#include "parser.h"

void run_pipeline(struct pipeline *pl);

void free_pipeline(struct pipeline *pl);

#endif