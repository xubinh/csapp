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

void helper_32_32(int A[32][32], int B[32][32], int begin_i, int begin_j, int size) {
    for (int i = begin_i; i < begin_i + size; i++) {
        for (int j = begin_j; j < begin_j + size; j++) {
            B[j][i] = A[i][j];
        }
    }

    return;
}

void trans_32_32(int A[32][32], int B[32][32]) {
    /**
     *
     * - cache 共 32 个组, 每个组共 1 行
     * - 一行共 32 字节, 等于 8 个 int 类型整数
     * - 矩阵的一行共 32 个整数, 等于 4 个 cache 行
     * - 一个 cache 等于 8 个矩阵行
     *
     */

    helper_32_32(A, B, 0, 24, 8);
    helper_32_32(A, B, 8, 16, 8);
    helper_32_32(A, B, 0, 16, 8);
    helper_32_32(A, B, 8, 24, 8);

    helper_32_32(A, B, 16, 0, 8);
    helper_32_32(A, B, 24, 8, 8);
    helper_32_32(A, B, 16, 8, 8);
    helper_32_32(A, B, 24, 0, 8);

    helper_32_32(A, B, 0, 8, 8);
    helper_32_32(A, B, 16, 24, 8);
    helper_32_32(A, B, 8, 0, 8);
    helper_32_32(A, B, 24, 16, 8);

    // 遍历所有以二维索引 (top_left, top_left) 为左上角顶点的 8 × 8 主对角块:
    for (int top_left = 0; top_left < 32; top_left += 8) {
        // 转置 A 的 8 × 8 主对角块的右上角 4 × 4 小块:
        for (int i = top_left; i < top_left + 4; i++) {
            for (int j = top_left + 4; j < top_left + 8; j++) {
                B[j][i] = A[i][j];
            }
        }

        // 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块:

        // - 使用局部变量存储 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左下角 2 × 2 小小块:
        int temp_0 = A[top_left + 2][top_left + 0];
        int temp_1 = A[top_left + 2][top_left + 1];
        int temp_2 = A[top_left + 2][top_left + 2];
        int temp_3 = A[top_left + 2][top_left + 3];

        // - 使用局部变量存储 A 的 8 × 8 主对角块的左上角 4 × 4 小块的右下角 2 × 2 小小块:
        int temp_4 = A[top_left + 3][top_left + 0];
        int temp_5 = A[top_left + 3][top_left + 1];
        int temp_6 = A[top_left + 3][top_left + 2];
        int temp_7 = A[top_left + 3][top_left + 3];

        // - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的右下角 2 × 2 小小块 (存储于局部变量中):
        B[top_left + 2][top_left + 2] = temp_2;
        B[top_left + 2][top_left + 3] = temp_6;
        B[top_left + 3][top_left + 2] = temp_3;
        B[top_left + 3][top_left + 3] = temp_7;

        // - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的右上角 2 × 2 小小块:
        B[top_left + 2][top_left + 0] = A[top_left + 0][top_left + 2];
        B[top_left + 2][top_left + 1] = A[top_left + 1][top_left + 2];
        B[top_left + 3][top_left + 0] = A[top_left + 0][top_left + 3];
        B[top_left + 3][top_left + 1] = A[top_left + 1][top_left + 3];

        // - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块:

        //   - 使用局部变量存储 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块的左下角 1 × 1 元素:
        int temp_8 = A[top_left + 1][top_left + 0];

        //   - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块的右下角 1 × 1 元素:
        B[top_left + 1][top_left + 1] = A[top_left + 1][top_left + 1];

        //   - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块的左上角 1 × 1 元素:
        B[top_left + 1][top_left + 0] = A[top_left + 0][top_left + 1];

        //   - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块的右上角 1 × 1 元素:
        B[top_left + 0][top_left + 0] = A[top_left + 0][top_left + 0];

        //   - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左上角 2 × 2 小小块的左下角 1 × 1 元素 (存储于局部变量中):
        B[top_left + 0][top_left + 1] = temp_8;

        // - 转置 A 的 8 × 8 主对角块的左上角 4 × 4 小块的左下角 2 × 2 小小块 (存储于局部变量中):
        B[top_left + 0][top_left + 2] = temp_0;
        B[top_left + 0][top_left + 3] = temp_4;
        B[top_left + 1][top_left + 2] = temp_1;
        B[top_left + 1][top_left + 3] = temp_5;

        // 以下是上述过程关于 A 的 8 × 8 主对角块的中心对称版本, 用于转置 A 的 8 × 8 主对角块的左下角和右下角 4 × 4
        // 小块:
        for (int i = top_left + 4; i < top_left + 8; i++) {
            for (int j = top_left; j < top_left + 4; j++) {
                B[j][i] = A[i][j];
            }
        }

        temp_0 = A[top_left + 4][top_left + 4];
        temp_1 = A[top_left + 4][top_left + 5];
        temp_2 = A[top_left + 4][top_left + 6];
        temp_3 = A[top_left + 4][top_left + 7];

        temp_4 = A[top_left + 5][top_left + 4];
        temp_5 = A[top_left + 5][top_left + 5];
        temp_6 = A[top_left + 5][top_left + 6];
        temp_7 = A[top_left + 5][top_left + 7];

        B[top_left + 4][top_left + 4] = temp_0;
        B[top_left + 4][top_left + 5] = temp_4;
        B[top_left + 5][top_left + 4] = temp_1;
        B[top_left + 5][top_left + 5] = temp_5;

        B[top_left + 4][top_left + 6] = A[top_left + 6][top_left + 4];
        B[top_left + 4][top_left + 7] = A[top_left + 7][top_left + 4];
        B[top_left + 5][top_left + 6] = A[top_left + 6][top_left + 5];
        B[top_left + 5][top_left + 7] = A[top_left + 7][top_left + 5];

        temp_8 = A[top_left + 6][top_left + 7];

        B[top_left + 6][top_left + 6] = A[top_left + 6][top_left + 6];

        B[top_left + 6][top_left + 7] = A[top_left + 7][top_left + 6];

        B[top_left + 7][top_left + 7] = A[top_left + 7][top_left + 7];

        B[top_left + 7][top_left + 6] = temp_8;

        B[top_left + 6][top_left + 4] = temp_2;
        B[top_left + 6][top_left + 5] = temp_6;
        B[top_left + 7][top_left + 4] = temp_3;
        B[top_left + 7][top_left + 5] = temp_7;
    }
}

void helper_64_64_normal(int A[64][64], int B[64][64], int top_left_A_i, int top_left_A_j, int top_left_B_i,
                         int top_left_B_j) {
    for (int i = 0; i < 4; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_B_i + j][top_left_B_j + i] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }

    int temp_00 = A[top_left_A_i + 0][top_left_A_j + 0];
    int temp_01 = A[top_left_A_i + 0][top_left_A_j + 1];
    int temp_02 = A[top_left_A_i + 1][top_left_A_j + 0];
    int temp_03 = A[top_left_A_i + 1][top_left_A_j + 1];

    int temp_04 = A[top_left_A_i + 0][top_left_A_j + 2];
    int temp_05 = A[top_left_A_i + 0][top_left_A_j + 3];
    int temp_06 = A[top_left_A_i + 1][top_left_A_j + 2];
    int temp_07 = A[top_left_A_i + 1][top_left_A_j + 3];

    int temp_08 = A[top_left_A_i + 2][top_left_A_j + 0];
    int temp_09 = A[top_left_A_i + 2][top_left_A_j + 1];
    int temp_10 = A[top_left_A_i + 3][top_left_A_j + 0];
    int temp_11 = A[top_left_A_i + 3][top_left_A_j + 1];

    for (int i = 4; i < 6; i++) {
        for (int j = 4; j < 8; j++) {
            B[top_left_B_i + j][top_left_B_j + i] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_B_i + j + 2][top_left_B_j + i + 4] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 6; j < 8; j++) {
            B[top_left_B_i + j][top_left_B_j + i] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }

    B[top_left_B_i + 2][top_left_B_j + 0] = temp_04;
    B[top_left_B_i + 2][top_left_B_j + 1] = temp_06;
    B[top_left_B_i + 3][top_left_B_j + 0] = temp_05;
    B[top_left_B_i + 3][top_left_B_j + 1] = temp_07;

    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            B[top_left_B_i + i][top_left_B_j + j] = B[top_left_B_i + i + 2][top_left_B_j + j + 4];
        }
    }

    for (int i = 6; i < 8; i++) {
        for (int j = 4; j < 6; j++) {
            B[top_left_B_i + j][top_left_B_j + i] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }

    B[top_left_B_i + 0][top_left_B_j + 0] = temp_00;
    B[top_left_B_i + 0][top_left_B_j + 1] = temp_02;
    B[top_left_B_i + 1][top_left_B_j + 0] = temp_01;
    B[top_left_B_i + 1][top_left_B_j + 1] = temp_03;

    B[top_left_B_i + 0][top_left_B_j + 2] = temp_08;
    B[top_left_B_i + 0][top_left_B_j + 3] = temp_10;
    B[top_left_B_i + 1][top_left_B_j + 2] = temp_09;
    B[top_left_B_i + 1][top_left_B_j + 3] = temp_11;

    for (int i = 4; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            B[top_left_B_i + j][top_left_B_j + i] = A[top_left_A_i + i][top_left_A_j + j];
        }
    }
}

void helper_helper_64_64_diagonal(int A[64][64], int B[64][64], int top_left_A_i, int top_left_A_j, int top_left_B_i,
                                  int top_left_B_j) {
    int temp_00;
    int temp_01;
    int temp_02;
    int temp_03;
    int temp_04;
    int temp_05;
    int temp_06;
    int temp_07;
    int temp_08;
    int temp_09;
    int temp_10;
    int temp_11;

    temp_00 = A[top_left_A_i + 0][top_left_A_j + 0];
    temp_01 = A[top_left_A_i + 0][top_left_A_j + 1];
    temp_02 = A[top_left_A_i + 1][top_left_A_j + 0];
    temp_03 = A[top_left_A_i + 1][top_left_A_j + 1];

    temp_04 = A[top_left_A_i + 2][top_left_A_j + 0];
    temp_05 = A[top_left_A_i + 2][top_left_A_j + 1];
    temp_06 = A[top_left_A_i + 3][top_left_A_j + 0];
    temp_07 = A[top_left_A_i + 3][top_left_A_j + 1];

    temp_08 = A[top_left_A_i + 2][top_left_A_j + 2];
    temp_09 = A[top_left_A_i + 2][top_left_A_j + 3];
    temp_10 = A[top_left_A_i + 3][top_left_A_j + 2];
    temp_11 = A[top_left_A_i + 3][top_left_A_j + 3];

    B[top_left_B_i + 2][top_left_B_j + 0] = A[top_left_A_i + 0][top_left_A_j + 2];
    B[top_left_B_i + 2][top_left_B_j + 1] = A[top_left_A_i + 1][top_left_A_j + 2];
    B[top_left_B_i + 3][top_left_B_j + 0] = A[top_left_A_i + 0][top_left_A_j + 3];
    B[top_left_B_i + 3][top_left_B_j + 1] = A[top_left_A_i + 1][top_left_A_j + 3];

    B[top_left_B_i + 2][top_left_B_j + 2] = temp_08;
    B[top_left_B_i + 2][top_left_B_j + 3] = temp_10;
    B[top_left_B_i + 3][top_left_B_j + 2] = temp_09;
    B[top_left_B_i + 3][top_left_B_j + 3] = temp_11;

    B[top_left_B_i + 0][top_left_B_j + 0] = temp_00;
    B[top_left_B_i + 0][top_left_B_j + 1] = temp_02;
    B[top_left_B_i + 1][top_left_B_j + 0] = temp_01;
    B[top_left_B_i + 1][top_left_B_j + 1] = temp_03;

    B[top_left_B_i + 0][top_left_B_j + 2] = temp_04;
    B[top_left_B_i + 0][top_left_B_j + 3] = temp_06;
    B[top_left_B_i + 1][top_left_B_j + 2] = temp_05;
    B[top_left_B_i + 1][top_left_B_j + 3] = temp_07;
}

void helper_64_64_diagonal(int A[64][64], int B[64][64], int top_left) {
    helper_helper_64_64_diagonal(A, B, top_left, top_left + 4, top_left + 4, top_left);
    helper_helper_64_64_diagonal(A, B, top_left + 4, top_left, top_left, top_left + 4);
    helper_helper_64_64_diagonal(A, B, top_left, top_left, top_left, top_left);
    helper_helper_64_64_diagonal(A, B, top_left + 4, top_left + 4, top_left + 4, top_left + 4);
}

void trans_64_64(int M, int N, int A[N][M], int B[M][N]) {
    for (int i = 0; i < 64; i += 8) {
        for (int j = 0; j < 64; j += 8) {
            if (i == j) {
                helper_64_64_diagonal(A, B, i);
            } else {
                helper_64_64_normal(A, B, i, j, j, i);
            }
        }
    }
}

void trans_61_67(int M, int N, int A[N][M], int B[M][N]) {
    trans(M, N, A, B);
}

char my_trans_desc[] = "My Trans";
void my_trans(int M, int N, int A[N][M], int B[M][N]) {
    if (M == 32 && N == 32) {
        trans_32_32(A, B);
    } else if (M == 64 && N == 64) {
        trans_64_64(M, N, A, B);
    } else if (M == 61 && N == 67) {
        trans_61_67(M, N, A, B);
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
