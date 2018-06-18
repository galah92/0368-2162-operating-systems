// @author  Gal Aharoni
// @date    06/2018

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUFFER_SIZE (1 << 10)
#define N_PRINTABLES (126 - 32 + 1)
#define BACKLOG (1 << 8)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

bool is_running = true;
unsigned int pcc_count[N_PRINTABLES];
unsigned int nthreads; // running threads counter
pthread_mutex_t nthreads_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nthreads_cv = PTHREAD_COND_INITIALIZER;

size_t handle_buffer(const char *buff, size_t length)
{
    size_t printable_count = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (buff[i] >= 32 && buff[i] <= 126)
        {
            printable_count++;
            __sync_fetch_and_add(pcc_count + buff[i] - 32, 1);
        }
    }
    return printable_count;
}

void* on_accept(void *args)
{
    int err;
    int client_fd = (int)(intptr_t)args;

    // get size of msg
    uint32_t nbytes_left;
    ssize_t nbytes = recv(client_fd, &nbytes_left, sizeof(nbytes_left), 0);
    if (nbytes == -1) handle_error("read");
    nbytes_left = ntohl(nbytes_left);

    // process msg
    char buff[BUFFER_SIZE];
    size_t printable_count = 0;
    do
    {
        nbytes = recv(client_fd, buff, MIN(nbytes_left, BUFFER_SIZE), 0);
        if (nbytes == -1) handle_error("read");
        nbytes_left -= nbytes;
        printable_count += handle_buffer(buff, nbytes);
    } while (nbytes_left);

    // send response and close connection with client
    printable_count = htonl(printable_count);
    nbytes = send(client_fd, &printable_count, sizeof(size_t), 0);
    if (nbytes == -1) handle_error("send");
    close(client_fd);
    
    // dec nthreads and exit
    err = pthread_mutex_lock(&nthreads_mtx);
    if (err) handle_error("pthread_mutex_lock");
    nthreads--;
    if (!nthreads) {
        err = pthread_cond_signal(&nthreads_cv);
        if (err) handle_error("pthread_cond_signal");
    }
    err = pthread_mutex_unlock(&nthreads_mtx);
    if (err) handle_error("pthread_mutex_unlock");
    pthread_exit(EXIT_SUCCESS);
}

void on_sigint(int signum)
{
    int err;
    
    // stop accepting new connections
    is_running = false;
    
    // wait for all running threads to finish
    err = pthread_mutex_lock(&nthreads_mtx);
    if (err) handle_error("pthread_mutex_lock");
    while (nthreads)
    {
        err = pthread_cond_wait(&nthreads_cv, &nthreads_mtx);
        if (err) handle_error("pthread_cond_wait");
    }
    err = pthread_mutex_unlock(&nthreads_mtx);
    if (err) handle_error("pthread_mutex_unlock");    
    
    // output and exit
    for (int i = 0; i < N_PRINTABLES; i++)
    {
        printf("char '%c' : %u times\n", 32 + i, pcc_count[i]);
    }
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int err;
    
    // [1] - process args
    if (argc < 2)
    {
        printf("USAGE: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const uint16_t port = htons(strtol(argv[1], NULL, 0));

    // [2] - register to SIGTERM
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    sigfillset(&action.sa_mask);
    action.sa_handler = on_sigint;
    err = sigaction(SIGINT, &action, NULL);
    if (err == -1) handle_error("sigaction");
    
    // [3] - init server
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) handle_error("socket");
    struct sockaddr_in saddr = { .sin_family = AF_INET, .sin_port = port };
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (err == -1) handle_error("bind");
    err = listen(sfd, BACKLOG);
    if (err == -1) handle_error("listen");
    
    while (true)
    {
        if (!is_running) continue;
        
        // [4] - accept new connection
        int client_fd = accept(sfd, NULL, NULL);
        if (client_fd == -1) handle_error("listen");
        
        // [5] - spwan on_accept thread
        pthread_t thread_fd;
        void *client_fd_arg = (void*)(intptr_t)client_fd;
        err = pthread_create(&thread_fd, NULL, on_accept, client_fd_arg);
        if (err) handle_error("pthread_create");
        
        // [6] - inc nthreads
        err = pthread_mutex_lock(&nthreads_mtx);
        if (err) handle_error("pthread_mutex_lock");
        nthreads++;
        err = pthread_mutex_unlock(&nthreads_mtx);
        if (err) handle_error("pthread_mutex_unlock");
    }

    return EXIT_SUCCESS;
}