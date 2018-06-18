// @author  Gal Aharoni
// @date    06/2018

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define URANDOM ("/dev/urandom")
#define BUFFER_SIZE (1 << 10)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int connect_to_server(const char *address, const char *port)
{
    int err;

    // assuming address is ip
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) handle_error("socket");
    struct sockaddr_in server = { .sin_family = AF_INET };
    server.sin_port = htons(strtoul(port, NULL, 0));;
    err = inet_pton(AF_INET, address, &(server.sin_addr.s_addr));
    if (err == -1) handle_error("inet_pton");
    if (err == 1) { // success
        err = connect(sfd, (struct sockaddr *)&server, sizeof(server));
        if (err == -1) handle_error("connect");
        return sfd;
    }
    // assuming address is hostname
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *result, *rp;
    err = getaddrinfo(address, port, &hints, &result);
    if (err) handle_error("getaddrinfo");
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        err = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (err != -1) break;
        close(sfd);
    }
    freeaddrinfo(result);
    if (rp) return sfd;
    handle_error("host error");
    return -1;
}

int main(int argc, char *argv[])
{
    int err;
    
    // [1] - process args
    if (argc < 4)
    {
        printf("USAGE: %s <host> <port> <length>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *address = argv[1];
    const char *port = argv[2];
    size_t nbytes_left = strtoul(argv[3], NULL, 0);

    // [2] - connect to server
    int sfd = connect_to_server(address, port);

    // [3] - get random buff
    int urandom_fd = open(URANDOM, O_RDONLY);
    if (urandom_fd < 0) handle_error("open");
    char buff[nbytes_left];
    err = read(urandom_fd, buff, nbytes_left);
    if (err != nbytes_left) handle_error("read");

    // [4] - send msg
    uint32_t msg_size = htonl(nbytes_left);
    ssize_t nbytes = send(sfd, &msg_size, sizeof(msg_size), 0);
    if (nbytes == -1) handle_error("send");
    do
    {
        nbytes = send(sfd, &buff, MIN(nbytes_left, BUFFER_SIZE), 0);
        if (nbytes == -1) handle_error("send");
        nbytes_left -= nbytes;
    } while (nbytes);

    // [5] - recv printable count
    uint32_t printable_count;
    nbytes = recv(sfd, &printable_count, sizeof(printable_count), 0);
    if (nbytes == -1) handle_error("read");

    // [6] - print and exit
    printf("# of printable characters: %u\n", ntohl(printable_count));
    return EXIT_SUCCESS;
}