#include <stdio.h>
#include <stdlib.h>

/* 
    The merge function takes in an array arr and three indices l, m, and r. 
  It assumes that the two subarrays arr[l..m] and arr[m+1..r] are already sorted, 
  and it merges them into a single sorted array arr[l..r]
*/
void merge(int arr[], int l, int m, int r) {
    int i, j, k;

    // Find sizes of two subarrays
    int left_size = m - l + 1;
    int right_size = r - m;

    // Create temporary arrays for left and right subarrays
    int *L = malloc(left_size * sizeof(int));
    int *R = malloc(right_size * sizeof(int));

    // size of first array
    int n1 = m - l + 1;

    // size of second array
    int n2 = r - m;

    // create temporary arrays you the size of the arrays
    //int L[n1], R[n2];

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
    The mergeSort function takes in an array arr and two indices l and r. 
  It recursively splits the array into smaller arrays and then merges them back together using the merge function.
*/
void mergeSort(int arr[], int l, int r) {
    if (l < r) {

        // middle term
        int m = l + (r - l) / 2;

        // sort first half
        mergeSort(arr, l, m);

        // sort second half
        mergeSort(arr, m + 1, r);

        // merge the two halfs
        merge(arr, l, m, r);
    }
}

int main(int argc, char *argv[]) {

    // how to use this script
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // open binary file
    FILE *fp;
    fp = fopen(argv[1], "rb");

    // when unable to open file
    if (fp == NULL) {
        printf("Unable to open file %s\n", argv[1]);
        return 1;
    }

    // count the number of integers
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp) / sizeof(int);
    fseek(fp, 0, SEEK_SET);

    // int arr[count];
    int *arr = malloc(size * sizeof(int));
    // reads one integer at a time from the binary file
    int count = fread(arr, sizeof(int), size, fp);

    if (count != size)
    {
        printf("Error: could not read all integers from file\n");
        return 1;
    }

    // close binary file
    fclose(fp);

    // merge sort
    mergeSort(arr, 0, size - 1);

    // print sorted array
    printf("Sorted array: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}

