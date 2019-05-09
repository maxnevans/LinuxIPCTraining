#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <semaphore.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

int main(int argc, char* argv[], char* envp)
{
    int shared = shm_open("shared_ipc_file", O_CREAT | O_RDWR, 0777);
    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    int ret_val = sem_init(mutex, TRUE, 1);
    if (ret_val != 0)
        return fprintf(stderr, "Error on semaphore creation!");

    if (fork())
    {
        if (fork())
        {
            // Root process
            //sleep(1);
            char buffer[256] = {0};
            size_t count_read = 0;
            while (!count_read)
            {
                sem_wait(mutex);
                lseek(shared, SEEK_SET, 0);
                count_read = read(shared, buffer, sizeof(char) * 256);
                sem_post(mutex);
            }
            printf("Root process read %lu bytes: %s\n", count_read, buffer);
            shm_unlink("shared_ipc_file");
            sem_close(mutex);
        }
        else
        {
            // Second child
            const char* str = "Hello Root process! This is child process 2!";
            const size_t len = strlen(str);
            sem_wait(mutex);
            size_t wrote = write(shared, str, sizeof(char) * len);
            sem_post(mutex);
            printf("Child process 2 finished after %s writting!\n", wrote == len ? "successful" : "failed");
        }
        
    }
    else
    {
        // First child
        printf("Child process 1 finished!\n");
    }
    
    return 0;
}