/*
Author: Aiden Nelson, EID: 800742353
Date of Start: 4/11/2023

Project Topic: 
    This program uses threads (UNIX) to control 5 individual threads with a self-made 
    interrupt handler/thread scheduler. This project also uses skills in MUTEX
    semaphores to ensure safe operation. 

    The project runs on an infinite loop (with console statements showing the progress through the program).

    The program will create 5 threads, each with a unique ID. Each thread will run a function that will
    print out some information, before switching to another thread after a number of times (when the 
    scheduler is woken up).

    Meanwhile, the main method will be waiting on a semaphore that will never be released. This will
    cause main to act like an infinite loop, but will allow the scheduler to run (as it uses alert).

    Thanks for trying it out, I hope you enjoy!

*/

// Includes
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

//Constants
#define SLOWDOWN_FACTOR 100000000 // Reduce number of printf's for running thread (1/100million)
#define SCHEDULE_INTERVAL 2 // 2 seconds between calls to scheduler

// Global Variables
pthread_t thread1, thread2, thread3, thread4, thread5;
pthread_mutex_t condition_mutex; //Mutex for all conditional variables

//Make conditional variables for each thread and one for the main thread
pthread_cond_t thread_1_condition;
pthread_cond_t thread_2_condition;
pthread_cond_t thread_3_condition;
pthread_cond_t thread_4_condition;
pthread_cond_t thread_5_condition;

//Create an array of conditional variables for each thread
pthread_cond_t * thread_condition_array[5] = {&thread_1_condition, &thread_2_condition, &thread_3_condition, &thread_4_condition, &thread_5_condition};

pthread_cond_t main_condition;


int schedule_vector[5] = {5,4,3,2,1}; //Vector to keep track of order of threads to be executed
int loops = 0; //Number of times the scheduler has been called (to keep track of what thread to switch to next)
bool goFlag = false; //Flag to keep track of whether the current thread should keep running

// Function Prototype
void clock_interrupt_handler(void);
void child_thread_routine(void *thread_id);

//Main
int main()
{   

    //Initialize Mutex and all Conditional Variables
    pthread_mutex_init(&condition_mutex, NULL);

    pthread_cond_init(&thread_1_condition, NULL);
    pthread_cond_init(&thread_2_condition, NULL);
    pthread_cond_init(&thread_3_condition, NULL);
    pthread_cond_init(&thread_4_condition, NULL);
    pthread_cond_init(&thread_5_condition, NULL);

    //Initialize the conditional variable for the main thread (to put it into a blocking state)
    pthread_cond_init(&main_condition, NULL);


    //Create Threads
    int id1 = 1;
    pthread_create(&thread1, NULL, child_thread_routine, &id1);
    int id2 = 2;
    pthread_create(&thread2, NULL, child_thread_routine, &id2);
    int id3 = 3;
    pthread_create(&thread3, NULL, child_thread_routine, &id3);
    int id4 = 4;
    pthread_create(&thread4, NULL, child_thread_routine, &id4);
    int id5 = 5;
    pthread_create(&thread5, NULL, child_thread_routine, &id5);


    //Set up the clock interrupt handler and the initial alarm
    signal(SIGALRM, clock_interrupt_handler);
    alarm(SCHEDULE_INTERVAL);

    //Wait on the semaphore that will never be released (to put main thread in blocking state)
    pthread_mutex_lock(&condition_mutex);
    printf("Main thread is waiting on semaphore... \n");
    pthread_cond_wait(&main_condition, &condition_mutex);
    pthread_mutex_unlock(&condition_mutex);
    

    return 0;
} 

//Function Definitions
void clock_interrupt_handler(void)
{
    //This function will be called every SCHEDULE_INTERVAL seconds by the OS
    //It will switch to the next thread, stored in the schedule_vector, stopping the previous one

    //Lock the mutex
    pthread_mutex_lock(&condition_mutex);

    //If this is the first time the scheduler has been called, 
    //signal the conditional variable for the first thread in the schedule_vector
    if (loops == 0){
        printf("\nFirst time scheduler has been called, signaling thread %d...\n", schedule_vector[0]);
        pthread_cond_signal(thread_condition_array[schedule_vector[0]-1]);
        loops ++;
        goFlag = true;
    }
    else {
        //Get the ID of the thread that is currently running
        int current_thread_id = schedule_vector[(loops-1) % 5];

        goFlag = false;

        printf("\nScheduler has woken up %d times, and is forcing thread %d to yield... \n", loops+1, current_thread_id);

        //Wait on the conditional variable for the thread that is currently running
        pthread_cond_wait(thread_condition_array[current_thread_id-1], &condition_mutex);

        //Get the ID of the thread that will be running next
        int next_thread_id = schedule_vector[loops % 5];

        //Increment the number of times the scheduler has been called
        loops++;

        printf("\nScheduler is now signaling thread %d to begin running... \n", next_thread_id);

        goFlag = true;

        //Signal the conditional variable for the thread that will be running next
        pthread_cond_signal(thread_condition_array[next_thread_id-1]);
    }

    //Set the next alarm
    alarm(SCHEDULE_INTERVAL);

    printf("\nScheduler is going to sleep...\n\n");

    //Unlock the mutex
    pthread_mutex_unlock(&condition_mutex);
}

void child_thread_routine(void *thread_id)
{
    //This function will be called by each thread
    //It will print out some information, then switch to another thread after a number of times (when the scheduler is woken up)

    //Get the ID of the thread
    int id = *(int *)thread_id;

    //Get the thread's conditional variable
    pthread_cond_t * my_condition = thread_condition_array[id-1];

    int my_counter = 0;

    //Declare the start of this child thread
    printf("Child thread: %d has started... \n", id);

    while(true){
        //Wait on the conditional variable for this thread, before which you signal it in case the scheduler has already been called
        // and is waiting on this thread's conditional variable

        pthread_mutex_lock(&condition_mutex);

        //Officially yield here to the scheduler
        pthread_cond_signal(my_condition);

        if (my_counter > 0){
            //Declare that this thread has yielded
            printf("\nChild thread: %d has yielded...\n", id);
        }

        //Wait on the conditional variable for this thread (to put it in a blocking state until the scheduler signals it)
        pthread_cond_wait(my_condition, &condition_mutex);  

        pthread_mutex_unlock(&condition_mutex);

        //Run the thread's code while the goFlag is true. Once it is not, that means the scheduler is waiting for this thread to yield
        while (goFlag){
            if((my_counter % SLOWDOWN_FACTOR) == 0){
                printf("Child thread: %d is running... \n", id);
            }
            my_counter++;
        }
    }

    return;
}
