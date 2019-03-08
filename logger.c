#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "logger.h"

/* Allocate a new Logger_t and return a pointer to it. The logger will record
logged text to the file specified by fileName. If the file could not be opened,
this function will return NULL. */
Logger_t * newLogger(char * fileName) {
    Logger_t * logger = malloc(sizeof(Logger_t));
    logger->f = fopen(fileName, "w");
    if (logger->f == NULL) {
        destroyLogger(logger);
        return NULL;
    } 
    return logger;
}

/* Deallocate the Logger_t data structure. */
void destroyLogger(Logger_t * logger) {
    if (logger->f != NULL) {
        fclose(logger->f);
    }
    free(logger);
}

/* Write a string to the Logger_t specified by logger. The string provided must
be NULL terminated. */
bool logText(Logger_t * logger, char * string) {
    fprintf(logger->f, "%s\n", string);
    return true; //pointless
}

/* Flush the logger's output buffer. This ensures that any pending writes are
flushed to the disk. */
void flushLogger(Logger_t * logger) {
    fflush(logger->f);
}

/* Test cases for logger functionality. */
void testLogger() {
    Logger_t * testLogger = newLogger("testlog.txt");
    assert(testLogger != NULL);
    char * str = calloc(256, 1);
    strcpy(str, "Here is some text.\n");
    logText(testLogger, str);
    flushLogger(testLogger);
    destroyLogger(testLogger);
}
