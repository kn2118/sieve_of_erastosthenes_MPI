#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>


int main(int argc, char** argv)
{
    double local_start, local_finish, local_elapsed, elapsed;
    int my_rank, comm_sz, N;
    N = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    MPI_Barrier(MPI_COMM_WORLD);
    local_start = MPI_Wtime();

    // divide number range between process
    int task_size = (int)ceil((N - 1) / (double)comm_sz);
    int local_a = 2 + my_rank * task_size;
    int local_b = fmin(N, (local_a + task_size - 1)); // inclusive
    int local_task_size = local_b - local_a + 1;

    printf("my_rank: %d ,local_task_size: %d , start: %d , end: %d \n", my_rank, local_task_size, local_a, local_b);

    // find primes within number range in integer array form (0 for not-prime, 1 for prime/unexplored)
    int primes_size = 0;
    int* primes = (int*)malloc(local_task_size * sizeof(int));

    for (int i = 0; i < local_task_size; i++) {
        primes[i] = 1; // mark all integers as unexplored
    }

    // sieve
    for (int i = 3; i <= sqrt(local_b); i += 2) {
        for (int j = local_a; j <= local_b; j++) {
            if (j == 3 || j == 2) {
                continue;
            }
            if (j % 2 == 0 || (j % i == 0 && j != i)) {
                primes[j - local_a] = 0;
            }
        }
    }

    for (int i = 0; i < local_task_size; i++) {
        primes_size += primes[i];
    }

    printf("my_rank: %d, completed sieve algorithm \n", my_rank);

    // convert to integer representation
    int* new_primes = (int*)malloc(primes_size * sizeof(int));
    int j = 0;
    for (int i = 0; i < local_task_size; i++) {
        if (primes[i] == 1) {
            new_primes[j] = local_a + i;
            printf("%d ", new_primes[j]);
            j++;
        }
    }
    free(primes);
    primes = new_primes;


    printf("my_rank: %d, completed conversion of prime array \n", my_rank);

    // send integer representation to process 0
    if (my_rank != 0) {
        MPI_Send(&primes_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(primes, primes_size, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }

    // concatenate primes into buffer
    int* b_sizes; //sizes of each prime array sent from processes
    int* b; // buffer
    int total_b_size; // size to initialize buffer

    if (my_rank == 0) {
        total_b_size = primes_size; //buffer size estimate

        b_sizes = (int*)malloc(comm_sz * sizeof(int)); // # primes from each process
        for (int q = 1; q < comm_sz; q++) {
            printf("waiting to receive process values %d \n", q);
            MPI_Recv(&b_sizes[q], 1, MPI_INT, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_b_size += b_sizes[q];
        }
        printf("total_size calculated: %d \n", total_b_size);

        b = (int*)malloc(total_b_size * sizeof(int)); //buffer
        for (int i = 0; i < primes_size; i++) { // populate buffer with process 0 items
            b[i] = primes[i];
        }
        printf("populated process 0 primes \n");

        int offset = primes_size;
        for (int q = 1; q < comm_sz; q++) { // populate buffer with non-o process items
            printf("waiting to receive primes from process %d with size %d and printing at offset %d \n", q, b_sizes[q], offset);
            MPI_Recv(b + offset, b_sizes[q], MPI_INT, q, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            offset += b_sizes[q];
        }
        printf("finished concatenating buffer of size: %d \n", total_b_size);
        free(b_sizes);
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;
    MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("Elapsed Time = %e seconds \n", elapsed);
        // create, name, and assign permissions to file
        FILE* fp;
        char filename[50];
        sprintf(filename, "%d.txt", N);
        fp = fopen(filename, "a");

        for (int i = 0; i < total_b_size; i++) {
            fprintf(fp, "%d ", b[i]); // non-zero process write to buffer
        }

        fclose(fp);
        free(b);
    }
    free(primes);
    MPI_Finalize();
    return 0;
}