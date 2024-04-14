# Implementation of Sieve of erastosthenes in MPI 
Kevin Ng
4/14/2024

1. Connect to server \\
` ssh crunchy1` 
2. Load MPI module \\ 
` module load mpi/openmpi-x86_64`
3. Compile MPI program \\  
` mpicc -g -Wall -std=c99 -o mpi_primes primes.c -lm ` 
4. Execute MPI Program (10 processes, N= 1000 for the example below) to get primes from 2 through 1000 \\ 
` mpiexec -n 10 ./mpi_primes 1000 `
