/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void my_trans(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    my_trans(M, N, A, B);
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

void helper_32_32_normal(int A[32][32], int B[32][32], int top_left_i, int top_left_j) {
    for (int i = top_left_i; i < top_left_i + 8; i++) {
        for (int j = top_left_j; j < top_left_j + 8; j++) {
            B[j][i] = A[i][j];
        }
    }
}

void helper_helper_32_32_diagonal(int A[32][32], int B[32][32], int top_left_i, int top_left_j) {
    if (top_left_i != top_left_j) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
            }
        }

        return;
    }

    int temp_00 = A[top_left_i + 0][top_left_j + 0];
    int temp_01 = A[top_left_i + 0][top_left_j + 1];
    int temp_02 = A[top_left_i + 1][top_left_j + 0];
    int temp_03 = A[top_left_i + 1][top_left_j + 1];

    int temp_04 = A[top_left_i + 2][top_left_j + 0];
    int temp_05 = A[top_left_i + 2][top_left_j + 1];
    int temp_06 = A[top_left_i + 3][top_left_j + 0];
    int temp_07 = A[top_left_i + 3][top_left_j + 1];

    int temp_08 = A[top_left_i + 2][top_left_j + 2];
    int temp_09 = A[top_left_i + 2][top_left_j + 3];
    int temp_10 = A[top_left_i + 3][top_left_j + 2];
    int temp_11 = A[top_left_i + 3][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 0] = A[top_left_i + 0][top_left_j + 2];
    B[top_left_j + 2][top_left_i + 1] = A[top_left_i + 1][top_left_j + 2];
    B[top_left_j + 3][top_left_i + 0] = A[top_left_i + 0][top_left_j + 3];
    B[top_left_j + 3][top_left_i + 1] = A[top_left_i + 1][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 2] = temp_08;
    B[top_left_j + 2][top_left_i + 3] = temp_10;
    B[top_left_j + 3][top_left_i + 2] = temp_09;
    B[top_left_j + 3][top_left_i + 3] = temp_11;

    B[top_left_j + 0][top_left_i + 0] = temp_00;
    B[top_left_j + 0][top_left_i + 1] = temp_02;
    B[top_left_j + 1][top_left_i + 0] = temp_01;
    B[top_left_j + 1][top_left_i + 1] = temp_03;

    B[top_left_j + 0][top_left_i + 2] = temp_04;
    B[top_left_j + 0][top_left_i + 3] = temp_06;
    B[top_left_j + 1][top_left_i + 2] = temp_05;
    B[top_left_j + 1][top_left_i + 3] = temp_07;
}

void helper_32_32_diagonal(int A[32][32], int B[32][32], int top_left) {
    helper_helper_32_32_diagonal(A, B, top_left, top_left + 4);
    helper_helper_32_32_diagonal(A, B, top_left, top_left);
    helper_helper_32_32_diagonal(A, B, top_left + 4, top_left);
    helper_helper_32_32_diagonal(A, B, top_left + 4, top_left + 4);
}

void trans_32_32(int A[32][32], int B[32][32]) {
    for (int top_left_i = 0; top_left_i < 32; top_left_i += 8) {
        for (int top_left_j = 0; top_left_j < 32; top_left_j += 8) {
            if (top_left_i == top_left_j) {
                helper_32_32_diagonal(A, B, top_left_i);
            } else {
                helper_32_32_normal(A, B, top_left_i, top_left_j);
            }
        }
    }
}

void helper_64_64_normal(int A[64][64], int B[64][64], int top_left_i, int top_left_j) {
    for (int i = 0; i < 4; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    int temp_00 = A[top_left_i + 0][top_left_j + 0];
    int temp_01 = A[top_left_i + 0][top_left_j + 1];
    int temp_02 = A[top_left_i + 1][top_left_j + 0];
    int temp_03 = A[top_left_i + 1][top_left_j + 1];

    int temp_04 = A[top_left_i + 0][top_left_j + 2];
    int temp_05 = A[top_left_i + 0][top_left_j + 3];
    int temp_06 = A[top_left_i + 1][top_left_j + 2];
    int temp_07 = A[top_left_i + 1][top_left_j + 3];

    int temp_08 = A[top_left_i + 2][top_left_j + 0];
    int temp_09 = A[top_left_i + 2][top_left_j + 1];
    int temp_10 = A[top_left_i + 3][top_left_j + 0];
    int temp_11 = A[top_left_i + 3][top_left_j + 1];

    for (int i = 4; i < 6; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_j + j + 2][top_left_i + i + 4] = A[top_left_i + i][top_left_j + j];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 6; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    B[top_left_j + 2][top_left_i + 0] = temp_04;
    B[top_left_j + 2][top_left_i + 1] = temp_06;
    B[top_left_j + 3][top_left_i + 0] = temp_05;
    B[top_left_j + 3][top_left_i + 1] = temp_07;

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_j + i][top_left_i + j] = B[top_left_j + i + 2][top_left_i + j + 4];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 4; j < 6; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    B[top_left_j + 0][top_left_i + 0] = temp_00;
    B[top_left_j + 0][top_left_i + 1] = temp_02;
    B[top_left_j + 1][top_left_i + 0] = temp_01;
    B[top_left_j + 1][top_left_i + 1] = temp_03;

    B[top_left_j + 0][top_left_i + 2] = temp_08;
    B[top_left_j + 0][top_left_i + 3] = temp_10;
    B[top_left_j + 1][top_left_i + 2] = temp_09;
    B[top_left_j + 1][top_left_i + 3] = temp_11;

    for (int i = 4; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }
}

void helper_helper_64_64_diagonal(int A[64][64], int B[64][64], int top_left_i, int top_left_j) {
    int temp_00 = A[top_left_i + 0][top_left_j + 0];
    int temp_01 = A[top_left_i + 0][top_left_j + 1];
    int temp_02 = A[top_left_i + 1][top_left_j + 0];
    int temp_03 = A[top_left_i + 1][top_left_j + 1];

    int temp_04 = A[top_left_i + 2][top_left_j + 0];
    int temp_05 = A[top_left_i + 2][top_left_j + 1];
    int temp_06 = A[top_left_i + 3][top_left_j + 0];
    int temp_07 = A[top_left_i + 3][top_left_j + 1];

    int temp_08 = A[top_left_i + 2][top_left_j + 2];
    int temp_09 = A[top_left_i + 2][top_left_j + 3];
    int temp_10 = A[top_left_i + 3][top_left_j + 2];
    int temp_11 = A[top_left_i + 3][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 0] = A[top_left_i + 0][top_left_j + 2];
    B[top_left_j + 2][top_left_i + 1] = A[top_left_i + 1][top_left_j + 2];
    B[top_left_j + 3][top_left_i + 0] = A[top_left_i + 0][top_left_j + 3];
    B[top_left_j + 3][top_left_i + 1] = A[top_left_i + 1][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 2] = temp_08;
    B[top_left_j + 2][top_left_i + 3] = temp_10;
    B[top_left_j + 3][top_left_i + 2] = temp_09;
    B[top_left_j + 3][top_left_i + 3] = temp_11;

    B[top_left_j + 0][top_left_i + 0] = temp_00;
    B[top_left_j + 0][top_left_i + 1] = temp_02;
    B[top_left_j + 1][top_left_i + 0] = temp_01;
    B[top_left_j + 1][top_left_i + 1] = temp_03;

    B[top_left_j + 0][top_left_i + 2] = temp_04;
    B[top_left_j + 0][top_left_i + 3] = temp_06;
    B[top_left_j + 1][top_left_i + 2] = temp_05;
    B[top_left_j + 1][top_left_i + 3] = temp_07;
}

void helper_64_64_diagonal(int A[64][64], int B[64][64], int top_left) {
    for (int i = 0; i < 8; i += 4) {
        for (int j = 0; j < 8; j += 4) {
            helper_helper_64_64_diagonal(A, B, top_left + i, top_left + j);
        }
    }
}

void trans_64_64(int A[64][64], int B[64][64]) {
    for (int top_left_i = 0; top_left_i < 64; top_left_i += 8) {
        for (int top_left_j = 0; top_left_j < 64; top_left_j += 8) {
            if (top_left_i == top_left_j) {
                helper_64_64_diagonal(A, B, top_left_i);
            } else {
                helper_64_64_normal(A, B, top_left_i, top_left_j);
            }
        }
    }
}

void helper_61_67_normal(int A[67][61], int B[61][67], int top_left_i, int top_left_j) {
    for (int i = 0; i < 4; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    int temp_00 = A[top_left_i + 0][top_left_j + 0];
    int temp_01 = A[top_left_i + 0][top_left_j + 1];
    int temp_02 = A[top_left_i + 1][top_left_j + 0];
    int temp_03 = A[top_left_i + 1][top_left_j + 1];

    int temp_04 = A[top_left_i + 0][top_left_j + 2];
    int temp_05 = A[top_left_i + 0][top_left_j + 3];
    int temp_06 = A[top_left_i + 1][top_left_j + 2];
    int temp_07 = A[top_left_i + 1][top_left_j + 3];

    int temp_08 = A[top_left_i + 2][top_left_j + 0];
    int temp_09 = A[top_left_i + 2][top_left_j + 1];
    int temp_10 = A[top_left_i + 3][top_left_j + 0];
    int temp_11 = A[top_left_i + 3][top_left_j + 1];

    for (int i = 4; i < 6; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_j + j + 2][top_left_i + i + 4] = A[top_left_i + i][top_left_j + j];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 6; j < 8; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    B[top_left_j + 2][top_left_i + 0] = temp_04;
    B[top_left_j + 2][top_left_i + 1] = temp_06;
    B[top_left_j + 3][top_left_i + 0] = temp_05;
    B[top_left_j + 3][top_left_i + 1] = temp_07;

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_j + i][top_left_i + j] = B[top_left_j + i + 2][top_left_i + j + 4];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 4; j < 6; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    B[top_left_j + 0][top_left_i + 0] = temp_00;
    B[top_left_j + 0][top_left_i + 1] = temp_02;
    B[top_left_j + 1][top_left_i + 0] = temp_01;
    B[top_left_j + 1][top_left_i + 1] = temp_03;

    B[top_left_j + 0][top_left_i + 2] = temp_08;
    B[top_left_j + 0][top_left_i + 3] = temp_10;
    B[top_left_j + 1][top_left_i + 2] = temp_09;
    B[top_left_j + 1][top_left_i + 3] = temp_11;

    for (int i = 4; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }
}

void helper_helper_61_67_diagonal(int A[67][61], int B[61][67], int top_left_i, int top_left_j) {
    int temp_00 = A[top_left_i + 0][top_left_j + 0];
    int temp_01 = A[top_left_i + 0][top_left_j + 1];
    int temp_02 = A[top_left_i + 1][top_left_j + 0];
    int temp_03 = A[top_left_i + 1][top_left_j + 1];

    int temp_04 = A[top_left_i + 2][top_left_j + 0];
    int temp_05 = A[top_left_i + 2][top_left_j + 1];
    int temp_06 = A[top_left_i + 3][top_left_j + 0];
    int temp_07 = A[top_left_i + 3][top_left_j + 1];

    int temp_08 = A[top_left_i + 2][top_left_j + 2];
    int temp_09 = A[top_left_i + 2][top_left_j + 3];
    int temp_10 = A[top_left_i + 3][top_left_j + 2];
    int temp_11 = A[top_left_i + 3][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 0] = A[top_left_i + 0][top_left_j + 2];
    B[top_left_j + 2][top_left_i + 1] = A[top_left_i + 1][top_left_j + 2];
    B[top_left_j + 3][top_left_i + 0] = A[top_left_i + 0][top_left_j + 3];
    B[top_left_j + 3][top_left_i + 1] = A[top_left_i + 1][top_left_j + 3];

    B[top_left_j + 2][top_left_i + 2] = temp_08;
    B[top_left_j + 2][top_left_i + 3] = temp_10;
    B[top_left_j + 3][top_left_i + 2] = temp_09;
    B[top_left_j + 3][top_left_i + 3] = temp_11;

    B[top_left_j + 0][top_left_i + 0] = temp_00;
    B[top_left_j + 0][top_left_i + 1] = temp_02;
    B[top_left_j + 1][top_left_i + 0] = temp_01;
    B[top_left_j + 1][top_left_i + 1] = temp_03;

    B[top_left_j + 0][top_left_i + 2] = temp_04;
    B[top_left_j + 0][top_left_i + 3] = temp_06;
    B[top_left_j + 1][top_left_i + 2] = temp_05;
    B[top_left_j + 1][top_left_i + 3] = temp_07;
}

void helper_61_67_diagonal(int A[67][61], int B[61][67], int top_left) {
    for (int i = 0; i < 8; i += 4) {
        for (int j = 0; j < 8; j += 4) {
            helper_helper_61_67_diagonal(A, B, top_left + i, top_left + j);
        }
    }
}

void helper_61_67_right_edge(int A[67][61], int B[61][67], int top_left_i, int top_left_j) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    int temp_0 = A[top_left_i + 0][top_left_j + 4];
    int temp_1 = A[top_left_i + 1][top_left_j + 4];
    int temp_2 = A[top_left_i + 2][top_left_j + 4];
    int temp_3 = A[top_left_i + 3][top_left_j + 4];

    for (int i = 4; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    for (int i = 4; i < 8; i++) {
        for (int j = 4; j < 5; j++) {
            B[top_left_j + j][top_left_i + i] = A[top_left_i + i][top_left_j + j];
        }
    }

    B[top_left_j + 4][top_left_i + 0] = temp_0;
    B[top_left_j + 4][top_left_i + 1] = temp_1;
    B[top_left_j + 4][top_left_i + 2] = temp_2;
    B[top_left_j + 4][top_left_i + 3] = temp_3;
}

void trans_61_67(int A[67][61], int B[61][67]) {
    for (int top_left_i = 0; top_left_i < 64; top_left_i += 8) {
        for (int top_left_j = 0; top_left_j < 56; top_left_j += 8) {
            if (top_left_i == top_left_j) {
                helper_61_67_diagonal(A, B, top_left_i);
            } else {
                helper_61_67_normal(A, B, top_left_i, top_left_j);
            }
        }
    }

    // for (int begin_i = 0; begin_i < 64; begin_i += 4) {
    //     for (int j = 60; j >= 56; j--) {
    //         for (int i = 0; i < 4; i++) {
    //             B[j][begin_i + i] = A[begin_i + i][j];
    //         }
    //     }
    // }

    for (int top_left_i = 0; top_left_i < 64; top_left_i += 8) {
        helper_61_67_right_edge(A, B, top_left_i, 56);
    }

    // for (int begin_j = 0; begin_j < 60; begin_j += 4) {
    for (int begin_j = 0; begin_j < 58; begin_j += 3) {
        for (int i = 64; i < 67; i++) {
            for (int j = 0; j < 3; j++) {
                B[begin_j + j][i] = A[i][begin_j + j];
            }
        }
    }

    // for (int i = 64; i < 67; i++) {
    for (int i = 64; i < 67; i++) {
        for (int j = 60; j < 61; j++) {
            B[j][i] = A[i][j];
        }
    }
}

char my_trans_desc[] = "My Trans";
void my_trans(int M, int N, int A[N][M], int B[M][N]) {
    if (M == 32 && N == 32) {
        trans_32_32(A, B);
    } else if (M == 64 && N == 64) {
        trans_64_64(A, B);
    } else if (M == 61 && N == 67) {
        trans_61_67(A, B);
    } else {
        printf("Unsupported matrix dimension\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
