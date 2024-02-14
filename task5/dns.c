#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


void simple_mul(const int32_t *a, const int32_t *b, int32_t *res, int n) {
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            int32_t r = a[i * n + k];
            for (int j = 0; j < n; ++j) {
                res[i * n + j] += r * b[k * n + j];
            }
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "wrong input\n");
        _exit(1);
    }

    const char *a_name = argv[1];
    const char *b_name = argv[2];
    const char *c_name = argv[3];
    

    MPI_Init(&argc, &argv);
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    int p_n = cbrt(mpi_size);
    int pi = mpi_rank / p_n / p_n;
    int pj = mpi_rank / p_n % p_n;
    int pk = mpi_rank % p_n;

    int color = pi * p_n + pj; 
    MPI_Comm k_comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, pk, &k_comm);


    double t1 = MPI_Wtime();

    int fdA;
    if (!(fdA = open(a_name, O_RDONLY))) {
        fprintf(stderr, "No file named %s!\n", a_name);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int n;
    read(fdA, &n, sizeof(n));

    int block_n = n / p_n;


    int32_t *a_block = calloc(block_n * block_n, sizeof(*a_block));
    
    for (int i = 0; i < block_n; ++i) {
        lseek(fdA, (1 + (pi * block_n + i) * n + pk * block_n) * sizeof(*a_block), SEEK_SET);                
        read(fdA, a_block + i * block_n, block_n * sizeof(*a_block));
    }

    close(fdA);

      

    int fdB;
    if (!(fdB = open(b_name, O_RDONLY))) {
        fprintf(stderr, "No file named %s!\n", b_name);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int32_t *b_block = calloc(block_n * block_n, sizeof(*b_block));

    for (int k = 0; k < block_n; ++k) {
        int offset = (1 + (pk * block_n + k) * n + pj * block_n) * sizeof(*b_block);
        pread(fdB, b_block + k * block_n, block_n * sizeof(*b_block), offset);
    }

    close(fdB);


    int32_t *c_block = calloc(block_n * block_n, sizeof(*c_block));
    simple_mul(a_block, b_block, c_block, block_n);

    free(a_block);
    free(b_block);

    int32_t *res_block = calloc(block_n * block_n, sizeof(*res_block));
    MPI_Reduce(c_block, res_block, block_n * block_n, MPI_INT32_T, MPI_SUM, 0, k_comm);

    free(c_block);


    int k_rank;
    MPI_Comm_rank(k_comm, &k_rank);

    MPI_Comm_free(&k_comm);
    

    MPI_Comm writers_comm;
    MPI_Comm_split(MPI_COMM_WORLD, k_rank, color, &writers_comm);


    if (!k_rank) {
        int writer_rank = color;

        
        int fdc;

        if(!writer_rank) {
            fdc = open(c_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            write(fdc, &n, sizeof(n));
        }

        MPI_Barrier(writers_comm);

        if (writer_rank) {
            fdc = open(c_name, O_WRONLY, 0644);
        }


        for (int line = 0; line < block_n; ++line) {
            int offset = (1 + (pi * block_n + line) * n + pj * block_n) * sizeof(int32_t);
            while (pwrite(fdc, res_block + line * block_n, 
                block_n * sizeof(*res_block), offset) != block_n * sizeof(*res_block));
        }
    
        MPI_Barrier(writers_comm);

        close(fdc);
       

        if (!k_rank && !color) {
            double t2 = MPI_Wtime();
            printf("Elapsed time = %lf s\n", t2 - t1);
        }
    }

    free(res_block);

    MPI_Finalize();

    return 0;
}
