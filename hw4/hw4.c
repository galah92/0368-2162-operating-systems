// @author  Gal Aharoni
// @date    05/2018

#include <stdio.h>      // printf()
#include <stdlib.h>     // exit()
#include <string.h>     // memset()
#include <fcntl.h>      // open(), close()
#include <unistd.h>     // read(), write()
#include <pthread.h>    // that's why we here

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUFF_SIZE (1 << 20)

int fout; // fd of output file
char shared_buff[BUFF_SIZE]; // write to fout
unsigned int nthreads; // num of *running* threads
unsigned int nreads = 0; // num threads that finish read each step
unsigned int max_nbytes = 0; // num bytes to write per step
unsigned int size = 0; // total num bytes written to fout
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void* reader_routine(void *fin_name)
{
    // [0] open input file, init buffer
    int fin = open(fin_name, O_RDONLY);
    if (fin < 0) handle_error("open");
    char buff[BUFF_SIZE]; // read from fin
    int nbytes; // bytes read()
    int err; // for syscalls failures

    do
    {
        // [1] read next chunk
        nbytes = read(fin, buff, BUFF_SIZE);
        if (nbytes < 0) handle_error("read");

        // [2] XOR data to shared_buff
        err = pthread_mutex_lock(&mtx);
        if (err) handle_error("pthread_mutex_lock");
        for (unsigned int j = 0; j < nbytes; j++) shared_buff[j] ^= buff[j];
        max_nbytes = nbytes > max_nbytes ? nbytes : max_nbytes;
        nreads++;

        // [3] if last thread of current chunk - write shared_buff to fout
        if (nreads == nthreads) // end of step
        {
            err = write(fout, shared_buff, max_nbytes);
            if (err != max_nbytes) handle_error("write");
            memset(shared_buff, 0, BUFF_SIZE); // clear shared_buff
            size += max_nbytes;
            max_nbytes = 0;
            nreads = 0;
            err = pthread_cond_broadcast(&cv); // init next step
            if (err) handle_error("pthread_cond_broadcast");
        }
        else
        {
            err = pthread_cond_wait(&cv, &mtx); // wait for end of step
            if (err) handle_error("pthread_cond_broadcast");
        }
        err = pthread_mutex_unlock(&mtx);
        if (err) handle_error("pthread_mutex_unlock");

    } while (nbytes == BUFF_SIZE);

    // [4] close input file and exit thread
    nthreads--;
    close(fin);
    pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
    // [1] print welcome message
    const char *fnout = argv[1];
    const unsigned int num = argc - 2; // first is "a.out", second is fnout
    printf("Hello, creating %s from %d input files\n", fnout, num);

    // [2] init all variables
    int err; // for lib functions failures
    memset(shared_buff, 0, BUFF_SIZE);
    pthread_t threads[num];

    // [3] open output file
    fout = open(fnout, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if (fout < 0) handle_error("open");

    // [4] create reader threads foreach input file
    for (unsigned int i = 0; i < num; i++)
    {
        err = pthread_create(&threads[i], NULL, reader_routine, argv[i + 2]);
        if (err) handle_error("pthread_create");
    }
    nthreads = num;

    // [5] wait for all reader threads to finish
    for (unsigned int i = 0; i < num; i++)
    {
        err = pthread_join(threads[i], NULL);
        if (err) handle_error("pthread_join");
    }

    // [6] close output file
    err = close(fout);
    if (err < 0) handle_error("close");

    // [7] print termination message
    printf("Created %s with size %d bytes\n", fnout, size);

    // [8] exit with code 0
    pthread_exit(NULL);
}