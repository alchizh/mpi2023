#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "wrong input\n");
        _exit(1);
    }

    char *eptr = NULL;
    int p_row = strtol(argv[1], &eptr, 10);
    int p_col = strtol(argv[2], &eptr, 10);

    const char *a_name = argv[3];
    const char *b_name = argv[4];
    const char *c_name = argv[5];
    

    MPI_Init(&argc, &argv);
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    int pi = mpi_rank / p_col, pj = mpi_rank % p_col;

    MPI_Finalize();
    return 0;

    double t1 = MPI_Wtime();

    int fdA;
    if (!(fdA = open(a_name, O_RDONLY))) {
        fprintf(stderr, "No file named %s!\n", a_name);
        _exit(1);
    }

    int m, n;
    read(fdA, &m, sizeof(m));
    read(fdA, &n, sizeof(n));


    int h_shift = pj * (n / p_col) + (pj < n % p_col ? pj : n % p_col);
    int v_shift = pi * (m / p_row) + (pi < m % p_row ? pi : m % p_row);

    int pv = m / p_row + (pi < m % p_row);
    int ph = n / p_col + (pj < n % p_col);


    // int32_t block[pv][ph];
    int32_t *block = malloc(pv * sizeof(int32_t *));

     for (int i = 0; i < pv; ++i) {
        block[i] = malloc(ph * sizeof(int32_t));
        lseek(fdA, (h_shift + (v_shift + i) * n + 2) * sizeof(int32_t), SEEK_SET); //+2=dim
        read(fdA, &block[i], ph * sizeof(int32_t));
    }

    close(fdA);


    int fdb = open(b_name, O_RDONLY);

    read(fdb, &n, sizeof(n));
    lseek(fdb,  (1 + h_shift) * sizeof(int32_t), SEEK_SET);

    int32_t b_part[ph];
    read(fdb, b_part, ph * sizeof(b_part[0]));

    close(fdb);


    int32_t c_part[m];
    memset(c_part, 0, m * sizeof(int32_t));

    for (int i = 0; i < pv; ++i) {
        for (int j = 0; j < ph; ++j) {
            c_part[v_shift + i] += block[i][j] * b_part[j];
        }
    }


    int32_t c_res[m]; 

    MPI_Reduce(c_part, c_res, m, MPI_INT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

    for (int rank = 0; rank < mpi_size; ++rank) {
        if (mpi_rank == rank) { 
            printf("rank %d\n pv %d ph %d\n hs %d vs %d\n pi %d pj %d\n\n", 
                mpi_rank, pv, ph, h_shift, v_shift, pi, pj);
            char tmpname[16];
            snprintf(tmpname, 10, "block%d", mpi_rank);
            int fdtmp = open(tmpname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            for (int i = 0; i < pv; ++i) {
                write(fdtmp, block[i], ph * sizeof(block[i][0]));
            }
            close(fdtmp);
        }
    }
    
    if (!mpi_rank) {
        int fdc = open(c_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fdc, &m, sizeof(m));
        write(fdc, c_res, m * sizeof(c_res[0]));
        close(fdc);

        double t2 = MPI_Wtime();
        printf("Elapsed time = %lf s\n", t2 - t1);
    }

    MPI_Finalize();

    return 0;
}
