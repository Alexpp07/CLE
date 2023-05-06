## How to execute prog2

#### Compile
```
mpicc sorting.c -o sorting -lm
```

#### Run
```
mpirun -np [number_processes] ./sorting [file_name]
```