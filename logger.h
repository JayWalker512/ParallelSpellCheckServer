/* See logger.c for function documentation. */

#ifndef EVENTLOGGER_H
#define EVENTLOGGER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct Logger_s {
    FILE *f;
} Logger_t;

Logger_t * newLogger(char * fileName);
void destroyLogger(Logger_t * logger);
bool logText(Logger_t * logger, char * string);
void flushLogger(Logger_t * logger);

void testLogger();

#endif /* EVENTLOGGER_H */

