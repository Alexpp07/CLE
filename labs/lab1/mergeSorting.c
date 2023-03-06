#include <stdio.h>
#include <stdlib.h>

void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;

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
    
    // how to user this script
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *fp;
    fp = fopen(argv[1], "r");

    if (fp == NULL) {
        printf("Unable to open file %s\n", argv[1]);
        return 1;
    }

    int num;
    int count = 0;
    // count total number of integers
    while (fscanf(fp, "%d", &num) != EOF) {
        count++;
    }

    fseek(fp, 0, SEEK_SET);

    int arr[count];
    int i = 0;
    // store the integers in an array
    while (fscanf(fp, "%d", &num) != EOF) {
        arr[i] = num;
        i++;
    }

    fclose(fp);

    // merge sort
    mergeSort(arr, 0, count - 1);

    // print sorted array
    printf("Sorted array: ");
    for (i = 0; i < count; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}
