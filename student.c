/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 5
 * Fall 2016
 *
 * This file contains the CPU scheduler for the simulation.
 * Name:
 * GTID:
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex; //for current process
static pthread_mutex_t qutex; //for ready queue
static pthread_cond_t cpu_running;
static pcb_t* rqHead;
static int mode; // mode = 1 for round robin, mode = 2 for priority
static int timeSlice;

static void addTail(pcb_t* toAdd);
static void removeHead();
static pcb_t* getHighestPriority();

int cpu_count;

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running
 *	process indexed by the cpu id. See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    //FIFO
    pcb_t* processToSchedule = rqHead;
    
    if(processToSchedule == NULL){
        //context_switch(cpu_id, NULL, -1);
    } else {
    if(mode == 2){
    processToSchedule = getHighestPriority();
    processToSchedule->state = PROCESS_RUNNING;
    } else {
    processToSchedule->state = PROCESS_RUNNING;
    removeHead();
        }
    }
    
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = processToSchedule;
    pthread_mutex_unlock(&current_mutex);
    if(mode == 1){
        context_switch(cpu_id, processToSchedule, timeSlice);
    } else {
    context_switch(cpu_id, processToSchedule, -1);
    }
    
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&qutex);
    
    while(rqHead == NULL) { //rqHead being null means empty queue
        pthread_cond_wait(&cpu_running, &qutex);
    }
    
    pthread_mutex_unlock(&qutex);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    
    pthread_mutex_lock(&current_mutex);
    pcb_t* processToPutBack = current[cpu_id]; //current process to put back in rq
    processToPutBack->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);
    addTail(processToPutBack); //put back in ready queue
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    
    pthread_mutex_lock(&current_mutex);
    pcb_t* current_process = current[cpu_id];
    current_process->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* terminating_process = current[cpu_id];
    terminating_process->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
    if(mode != 2) {
    process->state = PROCESS_READY;
    addTail(process);
    } else {

    process->state = PROCESS_READY;
    addTail(process);

    pthread_mutex_lock(&current_mutex);
    int lowPriority = 11;
    int chosenCpu;
    pcb_t* cpu;
    int foundIdle = 0;
    int i = 0;

    while(i < cpu_count && foundIdle != 1){
        cpu = current[i];
        if(cpu == NULL){
            foundIdle = 1;
            chosenCpu = 0;
        } else {
            int priority = cpu->static_priority;
            if(priority < lowPriority){
                lowPriority = priority;
                chosenCpu = i;
            }
        }
        i++;
    } // end while, chosenCpu is cpu to preempt
    if(foundIdle != 1){
        if(process->static_priority > lowPriority){
            force_preempt(chosenCpu);
        }
    }
    
    pthread_mutex_unlock(&current_mutex);
    
    } // end else
    
}


static void removeHead(){
    pthread_mutex_lock(&qutex);
    
    if(rqHead->next == NULL){
        rqHead = NULL;
    } else {
    rqHead = rqHead->next;
    }

    pthread_mutex_unlock(&qutex);
}

static void addTail(pcb_t* toAdd){
    pthread_mutex_lock(&qutex);
    pcb_t* current = rqHead;
    toAdd->next = NULL;
    
    
    if(rqHead == NULL){
        rqHead = toAdd;
    } else {
    
    while (current->next != NULL){
        current = current->next;
    }
        current->next = toAdd;
        
    }
    pthread_mutex_unlock(&qutex);
    pthread_cond_broadcast(&cpu_running);
}

static pcb_t* getHighestPriority(){

    pthread_mutex_lock(&qutex);
    pcb_t* high_pcb = rqHead;
    pcb_t* current = rqHead;

    if(current == NULL){
        return NULL;
    }
    
    int high = rqHead->static_priority;
    pcb_t* prev;

    while(current->next != NULL){
        int next_priority = current->next->static_priority;

        if(next_priority > high) {
            high = next_priority;
            high_pcb = current->next;
            prev = current;         
        }
        current = current->next;
    }
    if(high_pcb->next == NULL){
        if(high_pcb == rqHead){
            pthread_mutex_unlock(&qutex);
            return high_pcb;
        }
        prev->next = NULL;
        pthread_mutex_unlock(&qutex);
        return high_pcb;
    }
    if(high_pcb == rqHead){
        rqHead = rqHead->next;
        pthread_mutex_unlock(&qutex);
        return high_pcb;
    }
    
    prev->next = high_pcb->next;
    pthread_mutex_unlock(&qutex);
    return high_pcb;
}



/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    //int cpu_count;

    /* Parse command-line arguments */
    /*
    if (argc != 2)
    {
        fprintf(stderr, "CS 2200 Project 5 Fall 2016 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    */
    cpu_count = atoi(argv[1]);

    /* FIX ME - Add support for -r and -p parameters*/
    /********** TODO **************/



    /* Allocate the current[] array and its mutex */
    /*********** TODO *************/

    if(argc == 4){
        mode = 1;
        timeSlice = atoi(argv[3]);
    }
    if(argc == 3){
        mode = 2;
    }


    rqHead = NULL;
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);

    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&qutex, NULL);
    pthread_cond_init(&cpu_running, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


