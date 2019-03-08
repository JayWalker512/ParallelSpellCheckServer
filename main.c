#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "trie.h"
#include "sck.h"
#include "threadsafeQueue.h"
#include "logger.h"

struct Configuration_s {
    uint16_t port;
    char * dictionaryFileName;
    int numWorkers;
    bool bGoodConf;
};

/* Returns a Configuration_s struct which has been populated according to
the arguments provided to the function (argc and argv from main). 
The returned struct will have bGoodConf set to true if the configuration
parameters provided are valid, false otherwise. */
struct Configuration_s setConfiguration(int argc, char * argv[]) {
    static const char * defaultDict = "words"; //keep it in static program memory
    const uint16_t defaultPort = 2667;
    const int defaultNumWorkers = 4;
    struct Configuration_s conf;

    conf.port = defaultPort;
    conf.dictionaryFileName = (char *) defaultDict;
    conf.numWorkers = defaultNumWorkers;
    conf.bGoodConf = true;

    for (size_t i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-d") == 0) {
            /*This might be dangerous since argv exists on the stack,
             but it should be available throughout the life of the 
             program because it doesn't go out of scope until
             main() dies. */
            if (i + 1 >= argc) {
                conf.bGoodConf = false;
                return conf;
            }

            conf.dictionaryFileName = argv[i + 1];
            if (conf.dictionaryFileName == NULL) {
                conf.dictionaryFileName = (char *) defaultDict;
            }
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                conf.bGoodConf = false;
                return conf;
            }

            conf.numWorkers = (int) strtol(argv[i + 1], NULL, 10);
            if (conf.numWorkers < 1) {
                conf.numWorkers = defaultNumWorkers;
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                conf.bGoodConf = false;
                return conf;
            }

            conf.port = (uint16_t) strtol(argv[i + 1], NULL, 10);
            if (conf.port < 1) {
                conf.port = defaultPort;
            }
        } else {
            conf.bGoodConf = false;
        }
    }

    if (conf.bGoodConf) {
        printf("Using dictionary %s\n", conf.dictionaryFileName);
        printf("Starting %d worker threads\n", conf.numWorkers);
        printf("Listening on port %d\n", (int) conf.port);
    }

    return conf;
}

struct ThreadParams_s {
    ThreadsafeQueue_t * socketQueue;
    ThreadsafeQueue_t * logQueue;
    Trie_t * dictionary;
};

/* Worker function that interacts with a single connected client and handles any
spell checking requests. */
void * spellWorker(void * param) {
    struct ThreadParams_s params = *((struct ThreadParams_s *) param);

    //get a socket from a queue
    NetSocket_t * client = (NetSocket_t *) popThreadsafeQueue(params.socketQueue);

    while (1) {
        //read from socket and spellcheck
        SocketPayload_t * payload = readLineNetSocket(client); //readNetSocket(client, 255);
        if (payload != NULL) {
            if (payload->size > 0) {

                /* Need to malloc a new string to pass to log thread
                 Otherwise we have a race to free() in destroySocketPayload() */
                char * logStr = (char *) calloc(payload->size + 16, 1);
                char * responseStr = (char *) calloc(payload->size + 16, 1);
                if (stringExistsInTrie(params.dictionary, payload->data)) {
                    sprintf(logStr, "%s OK", payload->data);
                    sprintf(responseStr, "%s OK\n", payload->data);
                } else {
                    sprintf(logStr, "%s MISSPELLED", payload->data);
                    sprintf(responseStr, "%s MISSPELLED\n", payload->data);
                }
                writeNetSocket(client, responseStr, strlen(responseStr));
                pushThreadsafeQueue(params.logQueue, logStr);

                free(responseStr);
            }
        }

        //if client disconnects, get another socket (client).
        if (payload == NULL) {
            puts("Client disconnected, waiting for a new one...");
            destroyNetSocket(client);
            client = (NetSocket_t *) popThreadsafeQueue(params.socketQueue);
        } else {
            destroySocketPayload(payload);
        }
    }
    return NULL;
}

/* Worker function which handles writing output to a log on it's own thread
and in a thread-safe manner. */
void * logWorker(void * param) {
    struct ThreadParams_s params = *((struct ThreadParams_s *) param);

    //setup logger
    Logger_t * logger = newLogger("log.txt");
    if (logger == NULL) {
        puts("Couldn't open log file!");
        exit(EXIT_FAILURE);
    }

    //read from queue and log until the sun burns out
    while (1) {
        char * str = popThreadsafeQueue(params.logQueue);
        logText(logger, str);

        //this call effectively disables buffering, but it ensures our data is written.
        flushLogger(logger);

        free(str); //we expect log elements to be malloc'd strings; must free
    }

    destroyLogger(logger);

    return NULL;
}

int main(int argc, char * argv[]) {
    //test libraries on running system
    testLogger();
    testTrie();
    testSock();
    testThreadsafeQueue();

    //parse args and set configuration
    struct Configuration_s conf = setConfiguration(argc, argv);
    if (conf.bGoodConf == false) {
        puts("Invalid configuration. Please see below and in readme.txt for valid options.");
        const char *optionsString = "\t-t <number> : The number of worker threads to spawn. This also serves as an "
            "\n\t\tupper bound on the number of simultaneously connected clients."
            "\n\t\tThe default number of threads is 4."
            "\n\t-d <file>   : Dictionary file to use. Words should be listed one per line."
            "\n\t\tThe default dictionary is the included file \"words\"."
            "\n\t-p <number> : TCP port to listen for incoming connections on. Default is "
            "\n\t\tport 2667.";
        puts(optionsString);
        exit(EXIT_FAILURE);
    }

    //load dictionary from argv
    Trie_t * dictionary = newTrieFromDictionary(conf.dictionaryFileName);

    //setup thread pool
    ThreadsafeQueue_t * socketQueue = newThreadsafeQueue(conf.numWorkers);
    ThreadsafeQueue_t * logQueue = newThreadsafeQueue(4096);
    struct ThreadParams_s tParams;
    tParams.socketQueue = socketQueue;
    tParams.logQueue = logQueue;
    tParams.dictionary = dictionary;

    pthread_t workerThreads[conf.numWorkers];
    for (int i = 0; i < conf.numWorkers; i++) {
        if (pthread_create(&workerThreads[i], NULL, spellWorker, &tParams) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    pthread_t logThread;
    if (pthread_create(&logThread, NULL, logWorker, &tParams) != 0) {
        exit(EXIT_FAILURE);
    }

    //setup server socket
    NetSocket_t * server = newNetSocketServer(conf.port);
    listenNetSocket(server);

    //loop listen for incoming connections and enqueue them
    while (1) {
        pushThreadsafeQueue(socketQueue, acceptNetSocket(server));
        puts("Accepted a new connection.");
    }

    //clean up
    //TODO Would be nice to make the threads join instead of waiting for the
    //socket queue eternally
    destroyNetSocket(server);
    destroyTrie(dictionary);

    return 0;
}
