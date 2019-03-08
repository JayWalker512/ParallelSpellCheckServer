//See sck.c for function documentation.

#ifndef SCK_H
#define SCK_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>

#define DEFAULT_NETSOCKET_BACKLOG 4

typedef struct NetSocket_s {
    int socket_desc;
    struct sockaddr_in server;
    int errorNumber;
} NetSocket_t;

typedef struct SocketPayload_s {
    char * data;
    size_t capacity;
    int size; //need to accommodate EOF
} SocketPayload_t;

NetSocket_t * newNetSocketClient(char * address, uint16_t port);
bool connectNetSocket(NetSocket_t * socket);

NetSocket_t * newNetSocketServer(uint16_t port);
bool listenNetSocket(NetSocket_t * socket);
NetSocket_t * acceptNetSocket(NetSocket_t * serverSocket);

SocketPayload_t * readNetSocket(NetSocket_t * socket, size_t numBytes);
SocketPayload_t * readLineNetSocket(NetSocket_t * socket);
bool writeNetSocket(NetSocket_t * socket, char * bytes, size_t numBytes);

void destroyNetSocket(NetSocket_t * sock);

void destroySocketPayload(SocketPayload_t * payload);
char * getNetSocketError(NetSocket_t * socket);

void testSock();


#endif /* SCK_H */

