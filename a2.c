#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int number_threads;
int sem_id;
int count = 0;
int ok = 0; // var to stop th
pthread_mutex_t my_lock;
pthread_cond_t my_cond = PTHREAD_COND_INITIALIZER;  // condition var to wake up th13
pthread_cond_t my_cond2 = PTHREAD_COND_INITIALIZER; // if start before th13, wait for th 13 to stop

// decrement by 1 the semaphore sem_no in the semaphore set sem_id
void P(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, -1, 0};

    semop(sem_id, &op, 1);
}

// increment by 1 the semaphore sem_no in the semaphore set sem_id
void V(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, +1, 0};

    semop(sem_id, &op, 1);
}

// function for the threads from P3
void *execP3Thread(void *args)
{
    int th_id = *((int *)args);

    if (th_id == 4)
    {
        P(sem_id, 0); // put th4 in the queue
    }
    if (th_id == 3)
    {
        P(sem_id, 3);
    }
    info(BEGIN, 3, th_id); //mark the start of a thread

    if (th_id == 1)
    {
        V(sem_id, 0); //start the thread
        P(sem_id, 1); // put int the queue
    }

    info(END, 3, th_id); //mark the end of a thread
    if (th_id == 3)
    {
        V(sem_id, 4);
    }
    if (th_id == 4)
    {

        V(sem_id, 1); // start the thread
    }
    return NULL;
}

// function for the threads from P2

void *execP2Thread(void *args)
{
    int th_id = *((int *)args);

    if (th_id == 4)
    {
        P(sem_id, 4);
    }
    info(BEGIN, 2, th_id); // mark the begging of the threads

    info(END, 2, th_id); //mark the end of the threads

    if (th_id == 1)
    {
        V(sem_id, 3);
    }
    return NULL;
}

void *six_in_a_room(void *arg)
{
    int th_id = *((int *)arg);

    if (th_id == 13)
    {

        //do not let the th_13 to stop as long as there are not 6 in the room
        pthread_mutex_lock(&my_lock);

        //wait for another 5 threads
        while (count != 5)
        {
            pthread_cond_wait(&my_cond, &my_lock);
        }

        info(BEGIN, 8, th_id); //mark the begging of the threads

        info(END, 8, th_id); //mark the ending of the threads

        ok = 1; // variable set in order to let the threads to run

        pthread_cond_broadcast(&my_cond2); //wake up all the threads

        pthread_mutex_unlock(&my_lock);

        return NULL;
    }

    P(sem_id, 2);

    info(BEGIN, 8, th_id); //mark the begging of the threads

    pthread_mutex_lock(&my_lock);

    count++; // find out the number of the threads in the room

    pthread_cond_signal(&my_cond);

    while (ok == 0) //do not let the threads to run
    {
        pthread_cond_wait(&my_cond2, &my_lock);
    }

    pthread_mutex_unlock(&my_lock);

    info(END, 8, th_id); //mark the ending of the threads

    V(sem_id, 2);

    return NULL;
}

int main()
{

    init();

    info(BEGIN, 1, 0); //mark the begging of the P1

    pid_t p2, p3, p4, p5, p6, p7, p8;

    int proces_status;

    sem_id = semget(IPC_PRIVATE, 5, IPC_CREAT | 0600);

    //initialize the semaphore
    semctl(sem_id, 0, SETVAL, 0);
    semctl(sem_id, 1, SETVAL, 0);
    semctl(sem_id, 2, SETVAL, 5);
    semctl(sem_id, 3, SETVAL, 0);
    semctl(sem_id, 4, SETVAL, 0);

    if (sem_id < 0)
    {
        perror("Error creating the semaphore set");
        exit(2);
    }

    p2 = fork(); //create p2

    //check if created child
    if (p2 < 0)
    {
        perror("cannot create child");
        exit(1);
    }
    if (p2 == 0)
    {
        info(BEGIN, 2, 0); //mark the begging of the P2
        p3 = fork();       //create p3

        if (p3 < 0)
        {
            perror("cannot create p3");
            exit(1);
        }

        if (p3 == 0)
        {
            info(BEGIN, 3, 0); //mark the begging of the P3

            number_threads = 5;
            pthread_t threads[number_threads];
            int th_args[number_threads];
            void *status;

            //create the threads
            for (int i = 1; i <= number_threads; i++)
            {
                th_args[i] = i;
                pthread_create(&threads[i], NULL, execP3Thread, &th_args[i]);
            }

            //wait the threads to finish
            for (int i = 1; i <= number_threads; i++)
            {
                int s = pthread_join(threads[i], &status);
                if (s != 0)
                {
                    perror("Thread problem in P3");
                }
                free(status);
            }

            p7 = fork(); // create child p7
            if (p7 < 0)
            {
                perror("cannot create child p7");
                exit(1);
            }
            if (p7 == 0)
            {
                info(BEGIN, 7, 0); //mark the begging of the P7
                info(END, 7, 0);   //mark the end of the P7
            }
            if (p7 > 0)
            {
                waitpid(p2, &proces_status, 0);
                info(END, 3, 0); //mark the ending of the P3
            }
        }

        if (p3 > 0)
        {

            p5 = fork(); // create child p5
            if (p5 < 0)
            {
                perror("cannot create child p5");
                exit(1);
            }
            if (p5 == 0)
            {
                info(BEGIN, 5, 0); //mark the begging of the P5
                info(END, 5, 0);   // mark the end of the P5
            }
            if (p5 > 0)
            {
                waitpid(p5, &proces_status, 0);

                number_threads = 5;
                pthread_t threads[number_threads];
                int th_args[number_threads];
                void *status;

                //create the threads
                for (int i = 1; i <= number_threads; i++)
                {
                    th_args[i] = i;
                    pthread_create(&threads[i], NULL, execP2Thread, &th_args[i]);
                }
                //wait the threads to finish
                for (int i = 1; i <= number_threads; i++)
                {
                    int s = pthread_join(threads[i], &status);
                    if (s != 0)
                    {
                        perror("Thread problem in P2");
                    }
                    free(status);
                }

                waitpid(p3, &proces_status, 0);

                info(END, 2, 0); //mark the end of the P2
            }
        }
    }
    if (p2 > 0)

    {
        p4 = fork(); // create p4
        if (p4 < 0)
        {
            perror("cannot create p4");
            exit(1);
        }
        if (p4 == 0)
        {
            info(BEGIN, 4, 0); //mark the begging of the P4
            p6 = fork();       // create child p6
            if (p6 < 0)
            {
                perror("cannot create child");
                exit(1);
            }
            if (p6 == 0)
            {
                info(BEGIN, 6, 0); //mark the begging of the P6
                info(END, 6, 0);   // mark the end of the P6
            }
            if (p6 > 0)
            {
                p8 = fork(); // create child p8
                if (p8 < 0)
                {
                    perror("cannot create child");
                    exit(1);
                }
                if (p8 == 0)
                {
                    info(BEGIN, 8, 0); //mark the begging of the P8
                    number_threads = 47;
                    pthread_t threads[number_threads];
                    int th_args[number_threads];
                    void *status;

                    // Create the lock to provide mutual exclusion for the concurrent threads
                    if (pthread_mutex_init(&my_lock, NULL) != 0)
                    {
                        perror("Cannot initialize the lock");
                        exit(2);
                    }

                    //create the threads
                    for (int i = 1; i <= number_threads; i++)
                    {
                        th_args[i] = i;
                        pthread_create(&threads[i], NULL, six_in_a_room, &th_args[i]);
                    }

                    //wait the threads to finish
                    for (int i = 1; i <= number_threads; i++)
                    {
                        int s = pthread_join(threads[i], &status);
                        if (s != 0)
                        {
                            perror("Thread problem in P8");
                        }
                        free(status);
                    }

                    // Remove the lock
                    if (pthread_mutex_destroy(&my_lock) != 0)
                    {
                        perror("Cannot destroy the lock");
                        exit(8);
                    }

                    info(END, 8, 0); //mark the end of the P8
                }
                if (p8 > 0)
                {
                    waitpid(p6, &proces_status, 0);
                    waitpid(p8, &proces_status, 0);
                    info(END, 4, 0); //mark the end of the P4
                }
            }
        }

        if (p4 > 0)
        {
            waitpid(p4, &proces_status, 0);
            waitpid(p2, &proces_status, 0);
            semctl(sem_id, 0, IPC_RMID, 0); //destroy the semaphore
            info(END, 1, 0);                //mark the end of the P1
        }
    }
}
