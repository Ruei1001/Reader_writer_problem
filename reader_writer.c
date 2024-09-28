#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 1024  // Size of the shared memory
#define SEM_KEY 0x1234  // Semaphore key
#define SEM2_KEY 0x9487
#define SHARED_FILE "MAPTEST"
#define SHARED_MEM_KEY 5678

// Shared memory structure
struct shared_memory {
    char message[SHM_SIZE];
};

void writer(char *,struct shared_memory *shm, int semid, int sem2);
void reader(char *,struct shared_memory *shm, int semid, int sem2, int id);

void die(char *s)
{
    perror(s);
    exit(1);
}

int main() {

    /* Create shared memory */
    struct shared_memory * shm;

    char c;
    int share_mem_id;
    key_t shared_memory_key;
    char *the_shared_memory, *s;

    if ((share_mem_id = shmget(SHARED_MEM_KEY, SHM_SIZE, IPC_CREAT | 0666)) < 0)
        die("shmget");

    if ((the_shared_memory = shmat(share_mem_id, NULL, 0)) == (char *) -1)
        die("shmat");
    

    
    /*==========================================================================*
    *                      create and initial semephore  			            *
    *===========================================================================*/
   /* Create semaphores */
    int semid = semget(SEM_KEY, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    int semBWreader = semget(SEM2_KEY, 2, 0666 | IPC_CREAT);
    if (semBWreader == -1) {
        perror("semget");
        exit(1);
    }
    // Initialize semaphores
    semctl(semid, 0, SETVAL, 1);  // Semaphore 0 for writer (1 means writer can write)
    semctl(semid, 1, SETVAL, 0);  // Semaphore 1 for reader1 (0 means readers wait)

    semctl(semBWreader, 0, SETVAL, 1);
    /* Semaphore2 0  for there are reader use shared memory (0 means ther are a reader still reading)  */
    semctl(semBWreader, 1, SETVAL, 0); 


    pid_t parent = getpid();
    //printf("Process <%d> launched\n",parent);
    
    //writer(shm, semid,semBWreader);

    /*==========================================================================*
    *               	             spawn  			                        *
    *===========================================================================*/
    pid_t child1, child2;
    
    (child1 = fork()) && (child2 = fork());
    if (child1 == 0)
    {
        pid_t cur = getpid();
        printf("Process <%d> launched\n",cur);
        reader(the_shared_memory,shm, semid,semBWreader, 1);
    }
    else if (child2 == 0)
    {
        pid_t cur = getpid();
        printf("Process <%d> launched\n",cur);
        reader(the_shared_memory,shm, semid, semBWreader,2);
    }
    else
    {
        shm = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        if (shm == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        pid_t parent = getpid();
        printf("Process <%d> launched\n",parent);
        
        writer(the_shared_memory,shm, semid,semBWreader);
    }
    
    printf("terminated\n");

    /* Wait for child processes to terminate */
    /*for (int i = 0; i < 2; ++i) {
        wait(NULL);
    }*/

    /* clean */
    semctl(semid, 0, IPC_RMID);
    munmap(shm, sizeof(struct shared_memory));

    return 0;
}

void writer(char * shared, struct shared_memory *shm, int semid, int sem2) {
    struct sembuf sb;
    struct sembuf sb2;

    while (1) {
        /*==========================================================================*
        *               	       wait for write signal  			                *
        *===========================================================================*/
        /* wait for writer signal */
        sb.sem_num = 0;
        sb.sem_op = -1;  // Decrement semaphore 0 (lock)
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - lock writer");
            exit(1);
        }
        /*==========================================================================*
        *               	       lock the shared memory  			                *
        *===========================================================================*/
        char text;
        /* wait for the sharef memory to be available */
        sb2.sem_num = 0;
        sb2.sem_op = -1;  // Decrement semaphore 1 (lock)
        sb2.sem_flg = 0;
        if (semop(sem2, &sb2, 1) == -1) {
            perror("sem 2 semop - lock reader");
            exit(1);

        }
        //printf("Parent locked memory\n");
        system("ps");
        printf("Parent: Enter a message: (enter \"-\" to end)" );

        fflush(stdin);
        
        
        int count = 0;
        text = getchar();
        getchar();
        
        shared[0] = text;
        if (text == '-')
            return;
        fflush(stdin);
        
        /*==========================================================================*
        *               	       unlock shared memory  			                *
        *===========================================================================*/
        /* unlock shared memory */
        sb2.sem_num = 0;
        sb2.sem_op = 1;  // increament semaphore 1 (unlock)
        sb2.sem_flg = 0;

        if (semop(sem2, &sb2, 1) == -1) {
            perror("sem2 semop - lock reader");
            exit(1);
        }
        //printf("Parent unlocked mem\n");

        /*==========================================================================*
        *               	      signal child to read   			                *
        *===========================================================================*/
        // Signal the children to read the message
        sb.sem_num = 1;
        sb.sem_op = 2;  // Increment semaphore 1 by 2 (signal readers)
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - signal readers");
            exit(1);
        }

        // Wait for both children to read the message
        sb.sem_num = 0;
        sb.sem_op = -2;  // Decrement semaphore 0 by 2 (lock)
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - wait readers");
            exit(1);
        }

        // Signal the parent to write the next message
        sb.sem_num = 0;
        sb.sem_op = 1;  // Increment semaphore 0 (unlock)
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - signal writer");
            exit(1);
        }
    }
}

/* to avoid deadlock child1 has higher priority*/
void reader(char * shared, struct shared_memory *shm, int semid, int sem2, int id) {
    struct sembuf sb;
    struct sembuf sb2;
    //pid_t cur = getpid();
    //printf("Process <%d> launched\n",cur);
    while (1) {

        // Wait for the parent to write the message
        sb.sem_num = 1;
        sb.sem_op = -1;  // Decrement semaphore 1 (lock)
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - lock reader");
            exit(1);
        }
        //printf("Child %d locked writer signal\n", id);
        sb2.sem_num = 0;
        sb2.sem_op = -1;  // Decrement semaphore 1 (lock)
        sb2.sem_flg = 0;
        if (semop(sem2, &sb2, 1) == -1) {
            perror("sem 2 semop - lock reader");
            exit(1);

        }

        pid_t child_pid = getpid();
        if (shared[0] == '-')
            return;
        printf("Child : Process<%d> recieved<", child_pid);
        
        putchar(shared[0]);
        putchar('>');
        putchar('\n');
        //system("PS");

        sb2.sem_num = 0;
        sb2.sem_op = 1;  // increament semaphore 1 (unlock)
        sb2.sem_flg = 0;

        if (semop(sem2, &sb2, 1) == -1) {
            perror("sem2 semop - lock reader");
            exit(1);
        }
        //printf("Child %d unlocked mem\n", id);


        // Signal the parent that the message has been read
        sb.sem_num = 0;
        sb.sem_op = 1;  // Increment semaphore 0 (unlock)
        sb.sem_flg = 0;

        sleep(1);
        //wait(NULL);
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - signal writer");
            exit(1);
        }
    }
}
