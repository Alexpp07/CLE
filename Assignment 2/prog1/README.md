# Assignment 2 prog1

### How to run

Use the command below to compile the file:
```c
mpicc -Wall -O3 -o textProcessing textProcessing.c textProcessingFunctions.c
```

Then, just run the file generated:
```c
mpiexec -n (number_of_threads) textProcessing -f (files to be processed)
```

