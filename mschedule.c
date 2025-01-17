#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "shareddefs.h"
#include <time.h>
#include <stdint.h>
#include <math.h>  // Include math.h for log() function

struct LL* queues[MAX_NUMBER_OF_THREADS];
struct LL* finished;
pthread_mutex_t q_locks[MAX_NUMBER_OF_THREADS];
pthread_cond_t q_cvs[MAX_NUMBER_OF_THREADS];
pthread_mutex_t f_lock;
int thread_num;
int N;
char SAP;
char QS[3];
char ALG[5];
int INMODE;
char INFILE[MAX_FILE_NAME_LENGTH];
float T, T1, T2, L, L1, L2;
int PC;
int OUTMODE = 1;
int isOut = 0;
char OUTFILE[MAX_FILE_NAME_LENGTH];
int done = 0;
struct timespec start;

// File pointer for output
FILE *output_file = NULL;

// Function to create a new process
struct Process* createProcess(int pid, int cpu_id, int burstlen, int arv, int finish, int turnaround){
    struct Process* new_process = (struct Process*)malloc(sizeof(struct Process));
    if (!new_process) {
        return NULL; // Memory allocation failure
    }
    new_process->pid = pid;
    new_process->cpu_id = cpu_id;
    new_process->burstlen = burstlen;
    new_process->arv = arv;
    new_process->finish = finish;
    new_process->turnaround = turnaround;
    new_process->next = NULL;
    new_process->wait = 0;
    return new_process;
}

// Function to create a new linked list
struct LL* createLinkedList(){
    struct LL* new_list = (struct LL*)malloc(sizeof(struct LL));
    if (!new_list) {
        return NULL; // Memory allocation failure
    }
    new_list->load = 0;
    new_list->head = NULL;
    new_list->tail = NULL;
    return new_list;
}

// Function to pop a process from the linked list
struct Process* pop(struct LL* linkedlist){
    if (linkedlist->head == NULL) {
        return NULL; // List is empty
    }
    struct Process* head_process = linkedlist->head;
    linkedlist->head = head_process->next;
    linkedlist->load -= head_process->burstlen;
    if (linkedlist->head == NULL) {
        linkedlist->tail = NULL;
    }
    head_process->next = NULL; // Detach the process
    return head_process;
}

// Function to append a process to the linked list
void LLappend(struct LL* linkedlist, struct Process* process){
    if (linkedlist->head == NULL) {
        linkedlist->head = process;
        linkedlist->tail = process;
    } else {
        // Add to the end of the list
        linkedlist->tail->next = process;
        linkedlist->tail = process;
    }
    // Adjust the load
    linkedlist->load += process->burstlen;
}

// Function to insert a process into the linked list sorted by burst length
void LLsorted_insert(struct LL* linkedlist, struct Process* process){
    if (linkedlist->head == NULL || process->burstlen < linkedlist->head->burstlen) {
        // Insert at the head if list is empty or if the burstlen is smallest
        process->next = linkedlist->head;
        linkedlist->head = process;
        if (linkedlist->tail == NULL) {
            linkedlist->tail = process; // Set tail if it was NULL
        }
    } else {
        // Insert in sorted position
        struct Process* current = linkedlist->head;
        while (current->next != NULL && current->next->burstlen <= process->burstlen) {
            current = current->next;
        }
        process->next = current->next;
        current->next = process;
        // Update tail if inserted at the end
        if (process->next == NULL) {
            linkedlist->tail = process;
        }
    }
    // Adjust the load
    linkedlist->load += process->burstlen;
}

// Function to insert a process into the linked list sorted by PID
void LLsorted_insert_id(struct LL* linkedlist, struct Process* process){
    if (linkedlist->head == NULL || process->pid < linkedlist->head->pid) {
        // Insert at the head if list is empty or if the pid is smallest
        process->next = linkedlist->head;
        linkedlist->head = process;
        if (linkedlist->tail == NULL) {
            linkedlist->tail = process; // Set tail if it was NULL
        }
    } else {
        // Insert in sorted position
        struct Process* current = linkedlist->head;
        while (current->next != NULL && current->next->pid <= process->pid) {
            current = current->next;
        }
        process->next = current->next;
        current->next = process;

        // Update tail if inserted at the end
        if (process->next == NULL) {
            linkedlist->tail = process;
        }
    }
}

// Function to traverse the finished list and display process information
void traverse(struct LL *list) {
    struct Process *current = list->head;
    int count = 0;
    int total_turnaround = 0;
    int total_waiting = 0;
    // Print table header
    if (isOut) {
        fprintf(output_file, "%-4s %-4s %-8s %-4s %-6s %-11s %-9s\n",
                "pid", "cpu", "burstlen", "arv", "finish", "waitingtime", "turnaround");
    } else {
        printf("%-4s %-4s %-8s %-4s %-6s %-11s %-9s\n",
               "pid", "cpu", "burstlen", "arv", "finish", "waitingtime", "turnaround");
    }
    // Traverse the list and print each process's details
    while (current != NULL) {
        /*
        if (isOut) {
            fprintf(output_file, "%-4d %-4d %-8d %-4d %-6d %-11d %-9d\n",
                    current->pid, current->cpu_id, current->burstlen,
                    current->arv, current->finish, current->wait, current->turnaround);
        }
        */ 
        printf("%-4d %-4d %-8d %-4d %-6d %-11d %-9d\n",
                current->pid, current->cpu_id, current->burstlen,
                current->arv, current->finish, current->wait, current->turnaround);
        total_turnaround += current->turnaround;
        total_waiting = current->wait;
        count++;
        current = current->next;
    }
    // Calculate and print average turnaround time if there are processes
    if (count > 0) {
        double average_turnaround = (double)total_turnaround / count;
        double average_waiting = (double)total_waiting / count;
        printf("average turnaround time: %.2f ms\n", average_turnaround);
    }
}

// Function to calculate time difference in milliseconds
int calculate_time_diff_ms(struct timespec start, struct timespec curr)
{
    int diff_sec = curr.tv_sec - start.tv_sec;         // Difference in seconds
    int diff_nsec = curr.tv_nsec - start.tv_nsec;      // Difference in nanoseconds
    // Adjust if nanoseconds difference is negative
    if (diff_nsec < 0) {
        diff_sec -= 1;
        diff_nsec += 1000000000;  // Add 1 second (in nanoseconds) to diff_nsec
    }
    // Convert total time difference to milliseconds
    return (diff_sec * 1000) + (diff_nsec / 1000000);  // Convert seconds to ms and nanoseconds to ms
}

// CPU thread function
void *cpu_thread(void* id) {
    int t_id = *((int *)id);  // Retrieve thread ID
    free(id);  // Free allocated memory
    if (OUTMODE == 3) {
        if (isOut) {
            fprintf(output_file, "Thread with id %d active\n", t_id);
        } else {
            printf("Thread with id %d active\n", t_id);
        }
    }
    pthread_mutex_t* qt_lock;
    pthread_cond_t* qt_cv;
    struct Process* pp;
    struct LL* ready_q;
    int queue_index;

    if (SAP == 'S') {
        queue_index = 0;
    } else {
        queue_index = t_id - 1;
    }

    qt_lock = &q_locks[queue_index];
    qt_cv = &q_cvs[queue_index];
    ready_q = queues[queue_index];

    while (1) {
        if (OUTMODE == 3) {
            if (isOut) {
                fprintf(output_file, "Thread %d is waiting to pick from queue %d\n", t_id, queue_index);
            } else {
                printf("Thread %d is waiting to pick from queue %d\n", t_id, queue_index);
            }
        }

        pthread_mutex_lock(qt_lock);
        while (ready_q->load <= 0 && !done) {
            pthread_cond_wait(qt_cv, qt_lock);
        }

        if (ready_q->load <= 0 && done) {
            pthread_mutex_unlock(qt_lock);
            if (OUTMODE == 3) {
                if (isOut) {
                    fprintf(output_file, "Thread with id %d terminating\n", t_id);
                } else {
                    printf("Thread with id %d terminating\n", t_id);
                }
            }
            pthread_exit(NULL);
        }

        pp = pop(ready_q);
        if (OUTMODE == 3) {
            if (isOut) {
                fprintf(output_file, "Thread %d has picked from queue %d, burst item %d\n", t_id, queue_index, pp->pid);
            } else {
                printf("Thread %d has picked from queue %d, burst item %d\n", t_id, queue_index, pp->pid);
            }
        }
        pthread_mutex_unlock(qt_lock);

        int burstlen = pp->burstlen;
        struct timespec curr;
        clock_gettime(CLOCK_REALTIME, &curr);
        int timestamp = calculate_time_diff_ms(start, curr);
        if (OUTMODE == 2) {
            if (isOut) {
                fprintf(output_file, "time=%d, cpu=%d, pid=%d, burstlen=%d\n", timestamp, t_id, pp->pid, burstlen);
            } else {
                printf("time=%d, cpu=%d, pid=%d, burstlen=%d\n", timestamp, t_id, pp->pid, burstlen);
            }
        }

        usleep(burstlen * 1000);

        if (OUTMODE == 3) {
            if (isOut) {
                fprintf(output_file, "Thread %d has finished executing burst item %d\n", t_id, pp->pid);
            } else {
                printf("Thread %d has finished executing burst item %d\n", t_id, pp->pid);
            }
        }

        clock_gettime(CLOCK_REALTIME, &curr);
        timestamp = calculate_time_diff_ms(start, curr);
        int finish = timestamp;
        int cpu_id = t_id;
        int turnaround = finish - pp->arv;
        int waiting = turnaround - pp->burstlen;
        pp->finish = finish;
        pp->cpu_id = cpu_id;
        pp->turnaround = turnaround;
        pp->wait = waiting;

        if (OUTMODE == 3) {
            if (isOut) {
                fprintf(output_file, "Thread %d is waiting to append burst item %d to finished list.\n", t_id, pp->pid);
            } else {
                printf("Thread %d is waiting to append burst item %d to finished list.\n", t_id, pp->pid);
            }
        }
        pthread_mutex_lock(&f_lock);
        LLsorted_insert_id(finished, pp);
        if (OUTMODE == 3) {
            if (isOut) {
                fprintf(output_file, "Thread %d has appended burst item %d to finished list.\n", t_id, pp->pid);
            } else {
                printf("Thread %d has appended burst item %d to finished list.\n", t_id, pp->pid);
            }
        }
        pthread_mutex_unlock(&f_lock);
    }
    if (OUTMODE == 3) {
        if (isOut) {
            fprintf(output_file, "Thread with id %d terminating\n", t_id);
        } else {
            printf("Thread with id %d terminating\n", t_id);
        }
    }
}

int main(int argc, char *argv[]){
    int a = 1;
    char arg[3];  // Increased size to accommodate longer arguments
    while (a < argc){
        strcpy(arg, argv[a]);
        a++;
        if (strcmp(arg, "-n") == 0){
            N = atoi(argv[a]);
            a++;  // Move to the next argument
        }
        else if (strcmp(arg, "-a") == 0){
            SAP = *argv[a];
            a++;
            strcpy(QS, argv[a]);
            a++;  // Move to the next argument
        }
        else if (strcmp(arg, "-s") == 0){
            strcpy(ALG, argv[a]);
            a++;  // Move to the next argument
        }
        else if (strcmp(arg, "-i") == 0){
            INMODE = 0;
            strcpy(INFILE, argv[a]);
            a++;  // Move to the next argument
        }
        else if (strcmp(arg, "-r") == 0){
            INMODE = 1;
            T = strtof(argv[a++], NULL);
            T1 = strtof(argv[a++], NULL);
            T2 = strtof(argv[a++], NULL);
            L = strtof(argv[a++], NULL);
            L1 = strtof(argv[a++], NULL);
            L2 = strtof(argv[a++], NULL);
            PC = atoi(argv[a]);
            a++;  // Move to the next argument after PC
        }
        else if (strcmp(arg, "-m") == 0){
            OUTMODE = atoi(argv[a]);
            a++;  // Move to the next argument
        }
        else if (strcmp(arg, "-o") == 0){
            strcpy(OUTFILE, argv[a]);
            isOut = 1;
            a++;  // Move to the next argument
        } else{
            printf("Invalid argument provided. Exiting...\n");
            exit(EXIT_FAILURE);
        }
    }

    // Open output file if specified
    if (isOut) {
        output_file = fopen(OUTFILE, "w");
        if (output_file == NULL) {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
    }

    // Initialize mutexes
    pthread_mutex_init(&f_lock, NULL);

    // Create queues, initialize locks and condition variables
    for (int i = 0; i < N; i++){
        struct LL* ready_queue = createLinkedList();
        queues[i] = ready_queue;
        pthread_mutex_init(&q_locks[i], NULL);
        pthread_cond_init(&q_cvs[i], NULL);
    }

    // Create finished list
    finished = createLinkedList();

    // Create threads
    pthread_t thread_ids[MAX_NUMBER_OF_THREADS];
    for (int i = 0; i < N; i++){
        int *thread_id = malloc(sizeof(int));
        *thread_id = i + 1;
        int res = pthread_create(&thread_ids[i], NULL, cpu_thread, thread_id);
        if (res != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &start);

    // Seed the random number generator
    srand(time(NULL));

    if (INMODE == 0){
        // Read from input file
        FILE *file = fopen(INFILE, "r");
        if (file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }
        char buffer[256];
        int pid = 1;
        int next_insert = 0;
        while (fgets(buffer, sizeof(buffer), file) != NULL) {

            // Trim newline character
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            // Skip empty lines
            if (strlen(buffer) == 0) {
                continue;
            }
            char *type = strtok(buffer, " ");
            char *val_str = strtok(NULL, " ");
            int val = atoi(val_str);
            if (strcmp(type, "PL") == 0){
                // Create process
                struct timespec curr;
                clock_gettime(CLOCK_REALTIME, &curr);
                int timespec = calculate_time_diff_ms(start, curr);
                int cpu_id = 0;
                int burstlen = val;
                int arv = timespec;
                int finish = 0;
                int turnaround = 0;
                struct Process* burst_item = createProcess(pid, cpu_id, burstlen, arv, finish, turnaround);
                pid++;
                // Determine where to insert, which locks to acquire
                struct LL* main_queue;
                pthread_mutex_t* main_lock;
                pthread_cond_t* main_cond;
                int insert_index;
                if (SAP == 'S'){
                    // Use queue 0 as default queue
                    insert_index = 0;
                }
                else if (SAP == 'M'){
                    if (strcmp(QS, "RM") == 0){
                        // Insert at next position
                        insert_index = next_insert;
                        next_insert = (next_insert + 1) % N;
                    }
                    else if(strcmp(QS, "LM") == 0) {
                        // Determine index of queue with min load
                        int min_load_index = 0;
                        int min_load = queues[0]->load;
                        for(int i = 1; i < N; i++){
                            if (queues[i]->load < min_load){
                                min_load = queues[i]->load;
                                min_load_index = i;
                            }
                        }
                        insert_index = min_load_index;
                    }
                    else {
                        printf("Invalid queue selection method. Terminating.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    printf("Invalid scheduling approach. Terminating.\n");
                    exit(EXIT_FAILURE);
                }
                main_queue = queues[insert_index];
                main_lock = &q_locks[insert_index];
                main_cond = &q_cvs[insert_index];
                if(OUTMODE == 3){ // Print that the main thread is waiting to append the burst item to related queue
                    if (isOut) {
                        fprintf(output_file, "Main thread is waiting to add burst item %d to queue %d\n", burst_item->pid, insert_index);
                    } else {
                        printf("Main thread is waiting to add burst item %d to queue %d\n", burst_item->pid, insert_index);
                    }
                }
                // Acquire lock
                pthread_mutex_lock(main_lock);
                // Insert into queue
                if (strcmp(ALG, "FCFS") == 0){
                    // Append to tail
                    LLappend(main_queue, burst_item);
                }
                else if(strcmp(ALG, "SJF") == 0){
                    // Insert into a sorted linked list
                    LLsorted_insert(main_queue, burst_item);
                }
                else {
                    printf("Invalid scheduling algorithm. Terminating.\n");
                    pthread_mutex_unlock(main_lock);  // Unlock before exiting
                    exit(EXIT_FAILURE);
                }
                if (OUTMODE == 3){
                    // Print that the main thread has appended the burst item to related queue
                    if (isOut) {
                        fprintf(output_file, "Main thread has added burst item %d to queue %d\n", burst_item->pid, insert_index);
                    } else {
                        printf("Main thread has added burst item %d to queue %d\n", burst_item->pid, insert_index);
                    }
                }
                // Signal
                pthread_cond_broadcast(main_cond);
                // Release lock
                pthread_mutex_unlock(main_lock);
            }
            else if(strcmp(type, "IAT") == 0){
                // Sleep
                if (OUTMODE == 3) {
                    if (isOut) {
                        fprintf(output_file, "Main thread sleeping for %d milliseconds...\n", val);
                    } else {
                        printf("Main thread sleeping for %d milliseconds...\n", val);
                    }
                }
                usleep(val * 1000);
            } else{
                printf("Invalid input type. Terminating.\n");
                exit(EXIT_FAILURE);
            }
        }
        fclose(file);  // Close the file after reading

        // Indicate that no more processes will be added
        done = 1;
        // Wake up all threads in case they are waiting
        for (int i = 0; i < N; i++) {
            pthread_mutex_lock(&q_locks[i]);
            pthread_cond_broadcast(&q_cvs[i]);
            pthread_mutex_unlock(&q_locks[i]);
        }
        // Join all threads
        for(int i = 0; i < N; i++){
            pthread_join(thread_ids[i], NULL);
        }
    }
    else {
        // Generate random processes
        double lambda_T = 1.0 / T;
        double lambda_L = 1.0 / L;
        int pid = 1;
        int next_insert = 0;
        for (int i = 0; i < PC; i++) {
            

            // Generate random burst length
            int burst_length;
            do {
                double u = (double)rand() / (double)RAND_MAX;
                double y = (-1.0) * log(1.0 - u) / lambda_L;
                burst_length = (int)(y + 0.5); // Round to nearest integer
            } while (burst_length < L1 || burst_length > L2);

            // Create process
            struct timespec curr;
            clock_gettime(CLOCK_REALTIME, &curr);
            int timespec = calculate_time_diff_ms(start, curr);
            int cpu_id = 0;
            int arv = timespec;
            int finish = 0;
            int turnaround = 0;
            struct Process* burst_item = createProcess(pid, cpu_id, burst_length, arv, finish, turnaround);
            pid++;
            // Determine where to insert, which locks to acquire
            struct LL* main_queue;
            pthread_mutex_t* main_lock;
            pthread_cond_t* main_cond;
            int insert_index;
            if (SAP == 'S'){
                // Use queue 0 as default queue
                insert_index = 0;
            }
            else if (SAP == 'M'){
                if (strcmp(QS, "RM") == 0){
                    // Insert at next position
                    insert_index = next_insert;
                    next_insert = (next_insert + 1) % N;
                }
                else if(strcmp(QS, "LM") == 0) {
                    // Determine index of queue with min load
                    int min_load_index = 0;
                    int min_load = queues[0]->load;
                    for(int i = 1; i < N; i++){
                        if (queues[i]->load < min_load){
                            min_load = queues[i]->load;
                            min_load_index = i;
                        }
                    }
                    insert_index = min_load_index;
                }
                else {
                    printf("Invalid queue selection method. Terminating.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else {
                printf("Invalid scheduling approach. Terminating.\n");
                exit(EXIT_FAILURE);
            }
            main_queue = queues[insert_index];
            main_lock = &q_locks[insert_index];
            main_cond = &q_cvs[insert_index];
            if(OUTMODE == 3){ // Print that the main thread is waiting to append the burst item to related queue
                if (isOut) {
                    fprintf(output_file, "Main thread is waiting to add burst item %d to queue %d\n", burst_item->pid, insert_index);
                } else {
                    printf("Main thread is waiting to add burst item %d to queue %d\n", burst_item->pid, insert_index);
                }
            }
            // Acquire lock
            pthread_mutex_lock(main_lock);
            // Insert into queue
            if (strcmp(ALG, "FCFS") == 0){
                // Append to tail
                LLappend(main_queue, burst_item);
            }
            else if(strcmp(ALG, "SJF") == 0){
                // Insert into a sorted linked list
                LLsorted_insert(main_queue, burst_item);
            }
            else {
                printf("Invalid scheduling algorithm. Terminating.\n");
                pthread_mutex_unlock(main_lock);  // Unlock before exiting
                exit(EXIT_FAILURE);
            }
            if (OUTMODE == 3){
                // Print that the main thread has appended the burst item to related queue
                if (isOut) {
                    fprintf(output_file, "Main thread has added burst item %d to queue %d\n", burst_item->pid, insert_index);
                } else {
                    printf("Main thread has added burst item %d to queue %d\n", burst_item->pid, insert_index);
                }
            }
            // Signal
            pthread_cond_broadcast(main_cond);
            // Release lock
            pthread_mutex_unlock(main_lock);

            // Generate random interarrival time
            int interarrival_time;
            do {
                double u = (double)rand() / (double)RAND_MAX;
                double x = (-1.0) * log(1.0 - u) / lambda_T;
                interarrival_time = (int)(x + 0.5); // Round to nearest integer
            } while (interarrival_time < T1 || interarrival_time > T2);
            

            if (interarrival_time > 0) {
                // Sleep for interarrival time
                if (OUTMODE == 3) {
                    if (isOut) {
                        fprintf(output_file, "Main thread sleeping for %d milliseconds...\n", interarrival_time);
                    } else {
                        printf("Main thread sleeping for %d milliseconds...\n", interarrival_time);
                    }
                }
                usleep(interarrival_time * 1000);
            }
        }

        // Indicate that no more processes will be added
        done = 1;
        // Wake up all threads in case they are waiting
        for (int i = 0; i < N; i++) {
            pthread_mutex_lock(&q_locks[i]);
            pthread_cond_broadcast(&q_cvs[i]);
            pthread_mutex_unlock(&q_locks[i]);
        }
        // Join all threads
        for(int i = 0; i < N; i++){
            pthread_join(thread_ids[i], NULL);
        }
    }
    // Print final data
    pthread_mutex_lock(&f_lock);
    traverse(finished);
    pthread_mutex_unlock(&f_lock);

    // Close output file if opened
    if (isOut) {
        fclose(output_file);
    }

    // Cleanup (Optional: Free allocated memory, destroy mutexes and condition variables)
}