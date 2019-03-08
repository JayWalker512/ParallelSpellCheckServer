#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "sck.h"

/* Allocates a new SocketPayload_t data structure and returns a pointer to it. */
static SocketPayload_t * newSocketPayload(size_t numBytes) {
    SocketPayload_t * payload;
    payload = (SocketPayload_t *) malloc(sizeof (SocketPayload_t));
    payload->data = calloc(numBytes, 1);
    payload->capacity = numBytes;
    payload->size = 0;
    return payload;
}

/* Deallocates a SocketPayload_t data structure. */
void destroySocketPayload(SocketPayload_t * payload) {
    if (payload->data != NULL) {
        free(payload->data);
    }
    if (payload != NULL) {
        free(payload);
    }
}

/* Deallocates a NetSocket_t data structure. */
void destroyNetSocket(NetSocket_t * sock) {
    //for a more serious library, we would do thorough checking
    //of close conditions for the socket.
    close(sock->socket_desc);
    free(sock);
}

/* Allocates a NetSocket_t data structure and returns a pointer to it. */
static NetSocket_t * newNetSocket() {
    NetSocket_t * sock = (NetSocket_t *) malloc(sizeof (NetSocket_t));
    sock->errorNumber = 0;
    return sock;
}

/* Creates a TCP client socket using the provided parameters. Does not connect the 
 socket or take any additional action. Returns NULL on failure. */
NetSocket_t * newNetSocketClient(char * address, uint16_t port) {
    NetSocket_t * sock = newNetSocket();
    sock->socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->socket_desc < 0) {
        sock->errorNumber = errno;
        return sock;
    }

    sock->server.sin_addr.s_addr = inet_addr(address);
    sock->server.sin_family = AF_INET;
    sock->server.sin_port = htons(port);

    return sock;
}

/* Connects a TCP client socket that was previously created with newNetSocketClient
 to a server. Returns true/false on success/failure. */
bool connectNetSocket(NetSocket_t * socket) {
    if (connect(socket->socket_desc, (struct sockaddr *) &(socket->server), sizeof (socket->server)) < 0) {
        socket->errorNumber = errno;
        return false;
    }

    return true;
}

/* Allocates a NetSocket_t data structure which is configured for listening on the
provided port number. */
NetSocket_t * newNetSocketServer(uint16_t port) {
    NetSocket_t * sock = newNetSocket();
    sock->socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    int option = 1;
    setsockopt(sock->socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof (option));
    if (sock->socket_desc < 0) {
        sock->errorNumber = errno;
        return sock;
    }

    sock->server.sin_addr.s_addr = INADDR_ANY;
    sock->server.sin_family = AF_INET;
    sock->server.sin_port = htons(port);

    if (bind(sock->socket_desc, (struct sockaddr *) &(sock->server), sizeof (struct sockaddr_in)) < 0) {
        sock->errorNumber = errno;
        return sock;
    }

    return sock;
}

/* Start a NetSocket_t listening for connections on it's pre-configured port. */
bool listenNetSocket(NetSocket_t * socket) {
    if (listen(socket->socket_desc, 3) < 0) {
        socket->errorNumber = errno;
        return false;
    }
    return true;
}

/* Accept a connection request from the serverSocket which was listening. 
This returns a pointer to a newly allocated NetSocket_t which can be used to
communicate with the newly accepted connection. */
NetSocket_t * acceptNetSocket(NetSocket_t * serverSocket) {
    NetSocket_t * sock = newNetSocket();
    size_t sockaddrInSize = sizeof (struct sockaddr_in);
    sock->socket_desc = accept(serverSocket->socket_desc, (struct sockaddr *) &(sock->server), (socklen_t *) & sockaddrInSize);
    if (sock->socket_desc < 0) {
        sock->errorNumber = errno;
        return sock;
    }
    return sock;
}

/* Read numBytes from the referenced NetSocket_t socket and return a pointer to a newly
allocated SocketPayload_t data structure containing the read data. If no data
was available for reading, this function returns NULL. */ 
SocketPayload_t * readNetSocket(NetSocket_t * socket, size_t numBytes) {
    SocketPayload_t * payload = newSocketPayload(numBytes);
    payload->size = recv(socket->socket_desc, payload->data, numBytes, 0);
    if (payload->size < 0) {
        free(payload);
        payload = NULL;
        return payload;
    }
    return payload;
}

/* Read a full line (terminated by a newline character) from a NetSocket_t.
Returns a pointer to a newly allocated SocketPayload_t if the line was read,
otherwise returns NULL in the case of an error (for example, the socket was
disconnected) */
SocketPayload_t * readLineNetSocket(NetSocket_t * socket) {
    const size_t defaultPayloadSize = 256;
    SocketPayload_t * payload = newSocketPayload(defaultPayloadSize);
    char c = '\0';
    bool bReceiving = true;
    size_t curChar = 0;
    while (bReceiving) {
        int sizeIn = 0;
        sizeIn = recv(socket->socket_desc, &c, 1, 0);

        if (sizeIn == EOF || sizeIn == 0) {
            //client disconnected
            bReceiving = false;
            if (curChar > 0) { //we've read some bytes, return those only
                payload->data[curChar] = '\0';
                payload->size = curChar;
                return payload;
            } else { //if we've read 0 bytes, the socket is disconnected
                destroySocketPayload(payload);
                return NULL;
            }
        } else {
            if (c == '\n') {
                payload->data[curChar] = '\0';
                payload->size = curChar;
                return payload;
            }

            payload->data[curChar] = c;
            curChar++;

            //realloc if we've exceeded payload container capacity
            if (curChar == payload->capacity) {
                SocketPayload_t * newPayload = newSocketPayload(payload->capacity * 2);
                memcpy(newPayload->data, payload->data, payload->capacity);
                destroySocketPayload(payload);
                payload = newPayload;
            }
        }
    }
    return payload;
}

/* Write numBytes from bytes to the NetSocket_t socket provided. */
bool writeNetSocket(NetSocket_t * socket, char * bytes, size_t numBytes) {
    if (send(socket->socket_desc, bytes, numBytes, 0) < 0) {
        return false;
    }
    return true;
}

/* Return the string associated with the error code present on the referenced
NetSocket_t socket. */
char * getNetSocketError(NetSocket_t * socket) {
    size_t errStringLength = strlen(strerror(socket->errorNumber));
    char * errString = calloc(errStringLength + 1, 1);
    strcpy(errString, strerror(socket->errorNumber));
    return errString;
}

/* Test cases for socket functionality. */
void testSock() {
    NetSocket_t * serverSock;
    //assert((serverSock = newNetSocketServer(2667))->errorNumber == 0);
    serverSock = newNetSocketServer(39999);
    if (serverSock->errorNumber) {
        puts(getNetSocketError(serverSock));
    }
    assert(listenNetSocket(serverSock));

    /*NetSocket_t * serverToClientSock;
    assert((serverToClientSock = acceptNetSocket(serverSock))->errorNumber == 0);

    SocketPayload_t * payload = readNetSocket(serverToClientSock, 255);
    puts(payload->data);*/

    NetSocket_t * clientSock;
    assert((clientSock = newNetSocketClient("127.0.0.1", 39999))->errorNumber == 0);
    assert(connectNetSocket(clientSock));

    NetSocket_t * serverToClientSock;
    assert((serverToClientSock = acceptNetSocket(serverSock))->errorNumber == 0);

    //send and receive and ensure there were no errors
    assert(writeNetSocket(clientSock, "hello", 6) == true);
    SocketPayload_t * payload = readNetSocket(serverToClientSock, 255);
    assert(payload->size == 6);

    if (payload != NULL) {
        //puts(payload->data);
        assert(strcmp(payload->data, "hello") == 0);
    }

    destroyNetSocket(clientSock);
    destroyNetSocket(serverToClientSock);
    destroyNetSocket(serverSock);


    /*int socket_desc;
    struct sockaddr_in server;
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        puts("Couldn't create socket!");
    }
    
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(2667);
    
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        puts("Connection error...");
        puts(strerror(errno));
        return;
    }
    
    puts("Connected");
    
    const char * str = "This is some data";
    if (send(socket_desc, str, strlen(str), 0) < 0) {
        puts("Failed to send data");
        return;
    }

    return;*/
}
