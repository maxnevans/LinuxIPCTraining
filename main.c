#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/shm.h>
#include <error.h>

#define SHARED_FILE_NAME    "shared_ipc_file"

sem_t* mutex;
int shared_file;
pid_t* pids;
size_t* lines_ctl;

void root_sigusr1(int nsig)
{
    printf("SIGUSR1 recieved!\n");
}

void child_sigusr2(int sig)
{
    while (*lines_ctl < 1000)
    {
        sem_wait(mutex);
        size_t this_session_write = 1000 - *lines_ctl > 100 ? 100 : 1000 - *lines_ctl;

        for (int i = 0; i < this_session_write; i++)
        {
            char buffer[256];
            sprintf(buffer, "%ld %d %d\n", *lines_ctl + i + 1, getpid(), 19);
            size_t written = write(shared_file, buffer, strlen(buffer));
        }
        if (this_session_write)
        {
            fsync(shared_file);
            *lines_ctl += 100;
        }
        
        sem_post(mutex);
    }

    kill(pids[0], SIGUSR1);
    printf("Child process %d finished\n", getpid());

    exit(0);
}

size_t fd_getline(int fd, char* buffer, size_t count_read)
{
    int string_length = 0;
    for(int i = 0; i < count_read; i++)
    {
        char a = 0;
        size_t hb_read = read(fd, &a, sizeof(char));
        if (!hb_read)
            break;
        if (a == '\n')
        {
            buffer[i] = 0;
            break;
        }

        buffer[i] = a;
        string_length++;
    }
    return string_length;
}

int main(int argc, char* argv[], char** envp)
{
    shared_file = shm_open(SHARED_FILE_NAME, O_CREAT | O_TRUNC | O_RDWR, 0777);

    int shm_mutex_handler = shmget(IPC_PRIVATE, sizeof(sem_t), O_CREAT | 0777);
    mutex = shmat(shm_mutex_handler, 0, 0);
    sem_init(mutex, 1, 1);

    size_t pids_mem_size = sizeof(pid_t)*3;
    int shm_handler_pids = shmget(IPC_PRIVATE, pids_mem_size, O_CREAT | 0777);
    pids = shmat(shm_handler_pids, 0, 0);
    memset(pids, 0, pids_mem_size);

    size_t lines_ctl_mem_size = sizeof(size_t);
    int shm_handler_lines_ctl = shmget(IPC_PRIVATE, lines_ctl_mem_size, O_CREAT | 0777);
    lines_ctl = shmat(shm_handler_lines_ctl, 0, 0);
    memset(lines_ctl, 0, lines_ctl_mem_size);

    if (fork())
    {
        if (fork())
        {
            // Root process
            signal(SIGUSR1, root_sigusr1);
            signal(SIGUSR2, SIG_IGN);
            pids[0] = getpid();

            pid_t grp = getpgrp();

            while (!(pids[0] && pids[1] && pids[2])); // For syncing processes

            kill(-grp, SIGUSR2);

            size_t offset = 0;
            size_t count_read = 0;
            while (count_read < 1000)
            {
                sem_wait(mutex);
                lseek(shared_file, offset, SEEK_SET);
                for (int j = 0; j < 75; j++)
                {
                    char buffer[256] = {0};
                    size_t hbread = fd_getline(shared_file, buffer, 256);
                    if (hbread)
                    {
                        printf("%s\n", buffer);
                        count_read++;
                    }
                    else
                    {
                        break;
                    }
                    
                }
                offset = lseek(shared_file, 0, SEEK_CUR);
                lseek(shared_file, 0, SEEK_END);
                sem_post(mutex);
            }

            shm_unlink(SHARED_FILE_NAME);
            sem_close(mutex);
            printf("Root process finished\n");
            exit(0);
        }
        else
        {
            // Second child
            signal(SIGUSR2, child_sigusr2);
            pids[2] = getpid();
        }
        
    }
    else
    {
        // First child
        signal(SIGUSR2, child_sigusr2);
        pids[1] = getpid();
    }
    
    for (;;)
        sleep(100);

    return 0;
}
