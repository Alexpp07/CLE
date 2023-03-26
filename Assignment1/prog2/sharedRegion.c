#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

// External global variables
extern int num_threads;
extern int status_monitor_init;
extern int* status_workers;
extern int status_distributor;

/** @brief Array of work details for each of the worker threads. Indexed by the threads' application id */
static struct WorkStruct* work_array;

/** @brief Array of work requests for each of the worker threads. Indexed by the threads' application id */
static bool* request_array;

// Shared memory variables
/** @brief Array of integers  */
static int* integersArray;

/** @brief Number of integers */
static int num_integers;

/** @brief Name of the file  */
static char *filename; 

/** @brief Counter of the number of workers that finished the current sorting stage, 
 * so that the distributor can wait for all workers before continuing */
static int num_finished_threads = 0;

// State Control variables
/** @brief Workers' synchronization point for new work to be assigned */
static pthread_cond_t fifoWorkAssignment;

/** @brief Distributor's synchronization point for when the workers request work */
static pthread_cond_t fifoWorkRequested;

/** @brief Distributor's synchronization point for when the workers have finished sorting at this stage */
static pthread_cond_t fifoFinished;

/** @brief Locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t access_cr = PTHREAD_MUTEX_INITIALIZER;

/** @brief Flag that guarantees the monitor is initialized once and only once */
static pthread_once_t init = PTHREAD_ONCE_INIT;

struct WorkStruct {
    int* array;
    int num_integers_in_array;
    bool should_work;
};

static void monitorInitialize() {
    if ((work_array = malloc(num_threads * sizeof(struct WorkStruct))) == NULL
            || (request_array = malloc(num_threads * sizeof(bool))) == NULL) {
        fprintf(stderr, "error on allocating space for the work and request arrays\n");
        pthread_exit(&status_monitor_init);
    }

    for (int i = 0; i < num_threads; i++)
        request_array[i] = false;

    pthread_cond_init(&fifoWorkAssignment, NULL);
    pthread_cond_init(&fifoWorkRequested, NULL);
    pthread_cond_init(&fifoFinished, NULL);
}

/*  --------------------  WORKER FUNCTIONS  --------------------  */
void getWork(int id, struct WorkStruct* work) {
    if (pthread_mutex_lock(&access_cr)!=0){
        {
            perror("error on entering monitor(CF)"); /* save error in errno */
            int status = EXIT_FAILURE;
            pthread_exit(&status);
        }
    }
    pthread_once(&init, monitorInitialize);
    request_array[id] = true;
    pthread_cond_signal(&fifoWorkRequested);

    // If the request hasn't been fulfilled yet
    while (request_array[id]) {
        pthread_cond_wait(&fifoWorkAssignment, &access_cr);
    }
    *work = work_array[id];
    if ((pthread_mutex_unlock(&access_cr)) != 0)
    {
        perror("error on exiting monitor(CF)"); /* save error in errno */
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }
}

void saveWork() {
    if (pthread_mutex_lock(&access_cr)!=0){
        {
            perror("error on entering monitor(CF)"); /* save error in errno */
            int status = EXIT_FAILURE;
            pthread_exit(&status);
        }
    }
    pthread_once(&init, monitorInitialize);
    num_finished_threads++;
    pthread_cond_signal(&fifoFinished);
    if ((pthread_mutex_unlock(&access_cr)) != 0)
    {
        perror("error on exiting monitor(CF)"); /* save error in errno */
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }
}

/*  --------------------  DISTRIBUTOR FUNCTIONS  --------------------  */
void readFile() {
    if (pthread_mutex_lock(&access_cr)!=0){
        {
            perror("error on entering monitor(CF)"); /* save error in errno */
            int status = EXIT_FAILURE;
            pthread_exit(&status);
        }
    }
    pthread_once(&init, monitorInitialize);

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int res = fread(&num_integers, sizeof(int), 1, file);
    if (res != 1) {
        if (ferror(file)) {
            fprintf(stderr, "Invalid file format\n");
            exit(EXIT_FAILURE);
        }
        else if (feof(file)) {
            printf("Error: end of file reached\n");
            exit(EXIT_FAILURE);
        }
    }

    integersArray = (int*) malloc(num_integers * sizeof(int));

    while (true) {
        res = fread(integersArray, sizeof(int), num_integers, file);
        if (feof(file)) {
            break;
        }
        else if (ferror(file)) {
            printf("Invalid file format\n");
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);

    if ((pthread_mutex_unlock(&access_cr)) != 0)
    {
        perror("error on exiting monitor(CF)"); /* save error in errno */
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }
}

static int getWorkRequests(struct WorkStruct* work_to_distribute, int num_workers, int num_of_work_requested) {
    int requested_work = 0;
    
    for (int i = 0; i < num_threads; i++) {
        int work_to_distribute_idx = requested_work + num_of_work_requested;

        // No more work to be assigned, terminate
        if (work_to_distribute_idx >= num_workers)
            break;

        if (request_array[i]) {
            work_array[i] = work_to_distribute[work_to_distribute_idx];
            requested_work++;
            request_array[i] = false;
        }
    }
    return requested_work;
}

void distributeWork(struct WorkStruct* work_to_distribute, int n_workers) {
    if (pthread_mutex_lock(&access_cr)!=0){
        {
            perror("error on entering monitor(CF)"); /* save error in errno */
            int status = EXIT_FAILURE;
            pthread_exit(&status);
        }
    }
    pthread_once(&init, monitorInitialize);

    int workers_ready = 0;

    for (int i = 0; i < n_workers; i++)
        if (work_to_distribute[i].should_work)
            workers_ready++;

    int num_of_work_requested = 0;

    num_of_work_requested = getWorkRequests(work_to_distribute, n_workers, num_of_work_requested);

    pthread_cond_broadcast(&fifoWorkAssignment);
    while (num_of_work_requested < n_workers) {
        pthread_cond_wait(&fifoWorkRequested, &access_cr);
        num_of_work_requested += getWorkRequests(work_to_distribute, n_workers, num_of_work_requested);
        pthread_cond_broadcast(&fifoWorkAssignment);
    }

    // The number of workers is different from the workers that requested work
    while (num_finished_threads < workers_ready) {
        pthread_cond_wait(&fifoFinished, &access_cr);
    }

    num_finished_threads = 0;

    if ((pthread_mutex_unlock(&access_cr)) != 0)
    {
        perror("error on exiting monitor(CF)"); /* save error in errno */
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }
}

void defineSubArray(int num_subarray, int index_subarray, int** subarray, int* size_subarray) {
    *size_subarray = num_integers / num_subarray;
    *subarray = integersArray + index_subarray * (*size_subarray);
}


/*  --------------------  MAIN FUNCTIONS  --------------------  */
void storeFilename(char* fileName) {
    filename = fileName;
}

void verifyResults()
{
    printf("\n Final Verification\n");

    int i;
    for (i = 0; i < num_integers - 1; i++)
        if (integersArray[i] > integersArray[i + 1])
        {   
            printf("  Error on file %s!\n", filename);
            printf("  Error in position %d between element %d and %d\n",
                i, integersArray[i], integersArray[i + 1]);
            break;
        }
    if (i == (num_integers - 1))
        printf(" Everything is OK for file %s\n", filename);

}

void printFinalResults()
{
    printf(" Printing final results: ");
    int i;
    for (i = 0; i < num_integers; i++)
        printf(" %d ",integersArray[i]);
    printf("\n");
}