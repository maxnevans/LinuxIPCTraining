#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(int argc, char* argv[], char* envp)
{
    int shared = shm_open("shared_ipc_file", O_CREAT | O_RDWR, 0777);
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
                lseek(shared, SEEK_SET, 0);
                count_read = read(shared, buffer, sizeof(char) * 256);
            }
            printf("Root process read %lu bytes: %s\n", count_read, buffer);
            shm_unlink("shared_ipc_file");
        }
        else
        {
            // Second child
            const char* str = "Hello Root process! This is child process 2!";
            const size_t len = strlen(str);
            size_t wrote = write(shared, str, sizeof(char) * len);
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