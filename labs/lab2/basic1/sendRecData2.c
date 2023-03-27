#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int size, rank;
    char data[100], recData[100];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank of the calling process
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get the total number of processes

    snprintf(data, 100, "I am here (%d)!", rank);

    // Determine the neighboring processes
    int prev_rank = (rank - 1 + size) % size;
    int next_rank = (rank + 1) % size;

    // Send the message to the next process and receive a message from the previous process
    MPI_Sendrecv(data, strlen(data) + 1, MPI_CHAR, next_rank, 0,
                 recData, 100, MPI_CHAR, prev_rank, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    printf("Process %d received message: %s\n", rank, recData);

    MPI_Finalize();
    return EXIT_SUCCESS;
}