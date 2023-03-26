#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#include "constants.h"
#include "sharedRegion.c"


/** @brief Worker threads' function, which sorts an array of integers */
static void *worker(void *id);

/** @brief Distributor thread's function, which assigns work to workers */
static void *distributor(void *par);

/** @brief Sorting function, used to sort an array of integers */
void mergeSort(int* arr, int size);

void merge(int* arr, int l, int m, int r);


// Global variables

/** @brief Exit status of the monitor initialization */
int status_monitor_init;

/** @brief Array holding the exit status of the worker threads */
int* status_workers;

/** @brief Exit status of the distributor thread */
int status_distributor;

/** @brief Default number of threads. Command-line arguments can change this parameter */ 
int num_threads = 4;



int main (int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Error running the program!\n");
        exit(EXIT_FAILURE);
    }

    num_threads = atoi(argv[1]);

    char* filename = argv[2];

    storeFilename(filename);

    pthread_t* t_worker_id;         // workers internal thread id array
    pthread_t t_distributor_id;     // distributor internal thread id
    unsigned int* worker_id;        // workers application thread id array
    unsigned int distributor_id;    // distributor application thread id
    int* pStatus;                   // pointer to execution status
    int i;                          // counting variable

    // Allocate memory
    if ((t_worker_id = malloc(num_threads * sizeof(pthread_t))) == NULL
            || (worker_id = malloc(num_threads * sizeof(unsigned int))) == NULL
            || (status_workers = malloc(num_threads * sizeof(unsigned int))) == NULL) {
        fprintf(stderr, "error on allocating space to worker arrays\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_threads; i++)
        worker_id[i] = i;
    distributor_id = i;

    // Start counting time
    clock_t begin = clock();

    // Create Workers and Distributor
    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&t_worker_id[i], NULL, worker, &worker_id[i]) != 0) {
            fprintf(stderr, "error on creating worker thread");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_create(&t_distributor_id, NULL, distributor, &distributor_id) != 0) {
        fprintf(stderr, "error on creating distributor thread");
        exit(EXIT_FAILURE);
    }

    // Wait for Workers and Distributor to Terminate
    for (i = 0; i < num_threads; i++) {
        if (pthread_join(t_worker_id[i], (void *)&pStatus) != 0) {
            fprintf(stderr, "error on waiting for thread worker");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(t_distributor_id, (void *)&pStatus) != 0) {
        fprintf(stderr, "error on waiting for thread distributor");
        exit(EXIT_FAILURE);
    }

    // Stop counting time
    clock_t end = clock();

    // Execution time
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute", time_spent);

    // Verify sorting results
    verifyResults();

    // Print results
    //printFinalResults();

    exit(EXIT_SUCCESS);
}


void *distributor(void *par) {
    unsigned int id = *((unsigned int *) par);

    // Read file and get integers
    readFile();

    // Allocate memory for data struct
    struct WorkStruct* distributorWork = malloc(num_threads * sizeof(struct WorkStruct));

    // Working threads
    int num_working_threads;

    // Iterate every stage
    for (int stage = num_threads; stage > 0; stage = round(stage/2)) {

        if (stage == num_threads)
        {
            num_working_threads = stage;
        }
        else
        {
            num_working_threads = stage * 2;
        } 
        
        for (int work_id = 0; work_id < num_working_threads; work_id++) {

            distributorWork[work_id].should_work = work_id < stage;   

            if (distributorWork[work_id].should_work) {                
                defineSubArray(stage, work_id, &distributorWork[work_id].array, &distributorWork[work_id].num_integers_in_array);
            }
        }

        distributeWork(distributorWork, num_working_threads);
    }   

    distributorWork[0].should_work = false;
    distributeWork(distributorWork, 1);


    free(distributorWork);

    status_workers[id] = EXIT_SUCCESS;
    pthread_exit(&status_workers[id]);
}

void *worker(void *par) {
    unsigned int id = *((unsigned int *) par);
    struct WorkStruct workerWork;
    
    while (true) {

        // Get sub array
        getWork(id, &workerWork);
        
        // Break if there is no work to do
        if (!workerWork.should_work)
            break;

        // Sort sub array
        mergeSort(workerWork.array, workerWork.num_integers_in_array);

        // Save sub array
        saveWork();
    }

    status_workers[id] = EXIT_SUCCESS;
    pthread_exit(&status_workers[id]);
}


void merge(int* arr, int l, int m, int r) {
    int i, j, k;

    // size of first array
    int n1 = m - l + 1;

    // size of second array
    int n2 = r - m;

    // create temporary arrays you the size of the arrays
    int L[n1], R[n2];

    // values are copied into the arrays
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;

    // iterates through the two subarrays, comparing the values at each index and inserting them into the correct position in the final sorted array arr
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // any remaining elements in L or R are copied over to the final sorted array
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}


/*
    
*/
void mergeSort(int* arr, int n) {
    int curr_size;
    int left_start;

    // Merge subarrays in bottom-up manner
    for (curr_size = 1; curr_size <= n-1; curr_size = 2*curr_size) {
        // Pick starting point of different subarrays of current size
        for (left_start = 0; left_start < n-1; left_start += 2*curr_size) {
            // Find ending point of left subarray
            int mid = left_start + curr_size - 1;

            // Find ending point of right subarray
            int right_end = MIN(left_start + 2*curr_size - 1, n-1);

            // Merge subarrays arr[left_start...mid] and arr[mid+1...right_end]
            merge(arr, left_start, mid, right_end);
        }
    }
}