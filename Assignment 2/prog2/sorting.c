#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

#include "constants.h"

/** @brief Function to verify results  */
static bool verifyResults();

/** @brief Functions to merge and sort the array  */
void merge(int* arr, int l, int m, int r);
void mergeSort(int* arr, int n);

/** @brief Array of integers  */
static int* integersArray;

/** @brief Number of integers */
static int num_integers;

/** @brief Name of the file  */
static char *fileName;

/** @brief Array of partial integers  */
int* partial_array;

uint32_t previousPower2(uint32_t x);


int main(int argc, char *argv[]) {

    int rank, size; // rank and size
    bool resultsOK = false; // store if the final results are OK
    bool ready_2_sort = false; // is the program ready to sort?
    FILE* file; // file
    int res; // response from file

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);


    // printf("Process with rank %d initialization\n", rank);


    // Get and process command line arguments
    if (rank==0){
        if (argc<2){
            printf("Command is not recognized. Don't forget to enter file name!\n");
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
        fileName = argv[1];
        printf("FILE NAME - %s\n",fileName);
    }


    // Start counting time
    clock_t begin = clock();


    // If Distributor Process, read file
    if (rank==0){
        file = fopen(fileName, "rb");
        if (file == NULL) {
            fprintf(stderr, "Error opening file %s\n", fileName);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
        res = fread(&num_integers, sizeof(int), 1, file);
        if (res != 1) {
            if (ferror(file)) {
                fprintf(stderr, "Invalid file format\n");
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            else if (feof(file)) {
                printf("Error: end of file reached\n");
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }
    }


    // Broadcast number of integers
    MPI_Bcast(&num_integers, 1, MPI_INT, 0, comm);
    MPI_Barrier(comm);


    // Check the size of the array
    if (((num_integers) != 0) && (((num_integers) & ((num_integers) - 1)) != 0)) {
        fprintf(stderr, "Error: invalid file, array size must be a power of 2\n");
        fclose(file);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }


    // Allocate memory for full array and partial array
    integersArray = malloc(num_integers * sizeof(int));
    partial_array = malloc(num_integers * sizeof(int));
    if ((integersArray == NULL)||(partial_array == NULL)) {
        printf("Error: memory allocation failed\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }


    // If Distributor Process, read integers from file and store them in the array
    if (rank==0){
        while (true) {
            res = fread(integersArray, sizeof(int), num_integers, file);
            if (feof(file)) {
                break;
            }
            else if (ferror(file)) {
                printf("Invalid file format\n");
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }
        fclose(file); // close file
        ready_2_sort = true; // ready to sort
        printf("Number of integers - %d\n",num_integers);
    }


    // Broadcast ready_2_sort updated variable
    MPI_Bcast(&ready_2_sort, 1, MPI_C_BOOL, 0, comm);
    MPI_Barrier(comm);


    // If not ready to sort, exit
    if (!ready_2_sort) {
        printf("Process with rank %d stopped\n", rank);    
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }


    //printf("Number of integers - %d (rank %d)\n",num_integers, rank);


    // Broadcast the array of integers
    MPI_Bcast(integersArray, num_integers, MPI_INT, 0, comm);
    MPI_Barrier(comm);


    //printf(" Array (rank %d): %d %d\n", rank, integersArray[0],integersArray[1]);
    
    
    // Run the number of iterations needed
    if (size==1){
        mergeSort(integersArray, num_integers);
    } else{
        int iteration = 1;
        while (iteration < size) {
            MPI_Comm new_comm;
            int color = (rank < iteration) ? 0 : 1;
            MPI_Comm_split(comm, color, rank, &new_comm);
            if (color == 0) {
                int chunk_size = num_integers / iteration;
                MPI_Scatter(integersArray, chunk_size, MPI_INT, partial_array, chunk_size, MPI_INT, 0, new_comm);
                mergeSort(partial_array, chunk_size);
                MPI_Gather(partial_array, chunk_size, MPI_INT, integersArray, chunk_size, MPI_INT, 0, new_comm);
                comm = new_comm;
            } else {
                break;
            }
            iteration *= 2;
        }
    }


    // If Distributor Process, verify results
    if (rank==0){
        resultsOK = verifyResults();
        if (!resultsOK){
            printf("ERROR : final results are NOT OK.\n");
            //free(integersArray);
            //free(partial_array);
            printf("Process with rank %d finished working\n", rank);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
    }
    

    // Stop counting time
    clock_t end = clock();


    // If Distributor Process, print the execution time
    if (rank==0){
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("The program took %f seconds to execute\n", time_spent);
    }

    // Dealocate memory
    free(integersArray);
    free(partial_array);


    //printf("Process with rank %d finished working\n", rank);


    MPI_Finalize();
    exit(EXIT_SUCCESS);
}


/*
Function to verify if the integers in the final array are sorted correctly
*/
bool verifyResults(){
    printf("\n Final Verification\n");

    int i;
    for (i = 0; i < num_integers - 1; i++)
        if (integersArray[i] > integersArray[i + 1])
        {   
            printf("  Error on file %s!\n", fileName);
            printf("  Error in position %d between element %d and %d\n",
                i, integersArray[i], integersArray[i + 1]);
            return false;
        }
    if (i == (num_integers - 1))
        printf(" Everything is OK for file %s\n", fileName);
    return true;
}


/*
Function to merge
*/
void merge(int* arr, int l, int m, int r) {
    int i, j, k;

    // size of first array
    int n1 = m - l + 1;

    // size of second array
    int n2 = r - m;

    // create temporary arrays you the size of the arrays
    //int L[n1], R[n2];
    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));

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

    free(R);
    free(L);
}



/*
Function to merge sort 
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

uint32_t previousPower2(uint32_t x) {
    if (x == 0) {
        return 0;
    }
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x - (x >> 1);
}