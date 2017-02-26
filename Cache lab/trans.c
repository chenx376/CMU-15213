/* Yue Zhao (yzhao6)
 * Implementation description:
 * To transpose the matrix, I read 'waside-blocking.pdf' first and use the
 * pattern in the paper: seperate the matrix into blocks to avoid misses and
 * evictions. Also, the TA taught in recitation, the diagnal elements evict 
 * each other so that it is better to use register to temporarily store the
 * data the write it back after use other elements in the same line. 
 * We have 32 sets in the cache and each set have a block which can store 8
 * integers.
 * 
 * For case 1, matrix A is 32 * 32, we can directly seperate it 
 * into 8*8 blocks. As a result, the matix has 4*4 blocks, the cache can
 * store 4 blocks at once. As a result, for example, the fifth block will be
 * store in the same place as the first block, but at this time we have finish
 * the transposition for the first block, there will not be evictions.
 *
 * For case 2, matrix A is 64 * 64, if we simply use the same 8*8 block as
 * case 1, there will be addional evictions since there are 8*8 blocks in the
 * matrix. For example, the fifth row in the first block will be stored in the
 * same sets as the first row in the first block so that there will be a lot
 * more additional evitions. Thus I seperate the block again into four 4*4
 * blocks, the upper-left and lower-right one treat them as same as before.
 * We first store data of upper-right block in the upper-right block in 
 * transposition. When I deal with the lower left block, I use registers to
 * store data from each column in the same row of upper-right block, then store
 * data from each row in the same column, then set the elements in upper-right
 * block using values in matrix, finally set elements in the lower-left
 * block using values in matrix.
 *
 * For case3, matrix A is 61 * 67, I use the same loop as case 1, I have tried
 * different block size to obtain better performance, use the same loop as case
 * 1, finally I choose 20*20 as the block size.
 *
 * Language: C    
 */  

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {

    /* ii: row; jj: column; i: row in the block; j: coloum in the block. */
    int i, j, ii, jj;
    /* temp1 ~ temp8: temporarily store data in registers to avoid miss,
     * detail usage is different for each test case.
     */ 
    int temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8;

    REQUIRES(M > 0);
    REQUIRES(N > 0);    

    /* Case 1: Use 8*8 blocks to increase hits.
     * Deal with diagonal elements seperately to avoid evicts.
     */ 
    if (M == 32 && N == 32) {
        /* Use temp1 to set block size.
         * Use temp2 to store diagonal data.
         * Use temp3 to store diagonal index.
         */
        temp1 = 8;
        for (ii = 0; ii < N; ii += temp1) {
            for (jj = 0; jj < M; jj += temp1) {
                for (i = ii; i < ii + temp1; i++) {
                    for (j = jj; j < jj + temp1; j++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp2 = A[i][j];
                            temp3 = i;
                        }
                    }
                    if (ii == jj) {
                        B[temp3][temp3] = temp2;
                    }
                }
            }
        }
    /* Case 2: Use 8*8 blocks to increase hits.
     * To avoid additional evicts for 1st and 5th row,
     * seperate blocks into four 4*4 blocks.
     * Deal with upper-left and lower-right blocks as case 1,
     * when deal with upper-right and lower-right blocks,
     * store the upper-right elements as transposition in upper-right,
     * use 8 registers as temporarily storage to copy data back to avoid 
     * evicts.
     */
    } else if (M == 64 && N == 64) {      
        /* For upper-left and lower-right blocks,
         * use temp1 to store diagonal data,
         * use temp2 to store diagonal index;
         * for upper-right and lower-left blocks,
         * use temp1 ~ temp8 to temporarily store data to avoid miss.
         */
        for (ii = 0; ii < N; ii += 8) {
            for (jj = 0; jj < M; jj += 8) {
                // upper block
                for (i = ii; i < ii + 4; i++) {
                    for (j = jj; j < jj + 4; j++) {
                        B[j][i + 4] = A[i][j + 4];
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp1 = A[i][j];
                            temp2 = i;                        
                        }
                    }
                    if (ii == jj) {
                        B[temp2][temp2] = temp1;
                    }
                }
                // upper right & lower left
                for (j = jj + 4; j < jj + 8; j++) {
                    temp1 = B[j - 4][ii + 4];
                    temp2 = B[j - 4][ii + 5];
                    temp3 = B[j - 4][ii + 6];
                    temp4 = B[j - 4][ii + 7];
                    temp5 = A[ii + 4][j - 4];
                    temp6 = A[ii + 5][j - 4];
                    temp7 = A[ii + 6][j - 4];
                    temp8 = A[ii + 7][j - 4];
                    B[j - 4][ii + 4] = temp5;
                    B[j - 4][ii + 5] = temp6;
                    B[j - 4][ii + 6] = temp7;
                    B[j - 4][ii + 7] = temp8;
                    B[j][ii] = temp1;
                    B[j][ii + 1] = temp2;
                    B[j][ii + 2] = temp3;
                    B[j][ii + 3] = temp4;
                }
                // lower - right
                for (i = ii + 4; i < ii + 8; i++) {    
                    for (j = jj + 4; j < jj + 8; j++) {
                        if (i != j) {                       
                            B[j][i] = A[i][j];
                        } else {
                            temp1 = A[i][j];
                            temp2 = i;
                        }
                    }
                    if (ii == jj) {
                        B[temp2][temp2] = temp1;
                    }
                }
            }
        }
    /* Case 3: use 20 * 20 blocks to avoid hits.
     * Deal with diagonal elements seperately to avoid evicts.
     */    
    } else {      
        /* Use temp1 to set block size.
         * Use temp2 to store diagonal data.
         * Use temp3 to store diagonal index.
         */
        temp1 = 20;

        for (ii = 0; ii < N; ii += temp1) {
            for (jj = 0; jj < M; jj += temp1) {
                for (i = ii; i < ii + temp1 && i < N; i++) {
                    for (j = jj; j < jj + temp1 && j < M; j++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp2 = A[i][j];
                            temp3 = i;
                        }
                    }
                    if (ii == jj) {
                        B[temp3][temp3] = temp2;
                    }
                }
            }
        }
    }
    ENSURES(is_transpose(M, N, A, B));
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

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * trans_32 - Transpotation for 32 * 32 matrix, block size is 8*8.
 */
char trans_32_desc[] = "For 32 * 32 Matrix";
void trans_32(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj; //jj: column ii: row, i,j for block
    int temp, diagonal;
    int block = 8;

    REQUIRES(M > 0);
    REQUIRES(N > 0);    

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            for (i = ii; i < ii + block; i++) {
                for (j = jj; j < jj + block; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }
        }
    }
    ENSURES(is_transpose(M, N, A, B));    
}

/*
 * trans_64 - Transpotation for 64 * 64 matrix, only seperate the 8*8 block 
 *            into four small blocks.
 */
char trans_64_desc[] = "For 64 * 64 Matrix";
void trans_64(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj; //jj: column ii: row, i,j for block
    int temp, diagonal;
    int block = 8;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            // upper - left
            for (i = ii; i < ii + block / 2; i++) {
                for (j = jj; j < jj + block / 2; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }

            //upper - right
            for (i = ii; i < ii + block / 2; i++) {
                for (j = jj + block / 2; j < jj + block; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }
            
            //lower - left
            for (i = ii + block / 2; i < ii + block; i++) {
                for (j = jj; j < jj + block / 2; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }
            
            //lower - right
            for (i = ii + block / 2; i < ii + block; i++) {
                for (j = jj + block / 2; j < jj + block; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }
        }
    }
    ENSURES(is_transpose(M, N, A, B));    
}


/*
 * trans_61_67 - Transpotation for 64 * 64 matrix, use 20*20 blocks.
 */
char trans_61_67_desc[] = "For 61 * 67 Matrix";
void trans_61_67(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj; //jj: column ii: row, i,j for block
    int temp, diagonal;
    int block = 20;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            for (i = ii; i < ii + block && i < N; i++) {
                for (j = jj; j < jj + block && j < M; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp = A[i][j];
                        diagonal = i;
                    }
                }
                if (ii == jj) {
                    B[diagonal][diagonal] = temp;
                }
            }
        }
    }
    ENSURES(is_transpose(M, N, A, B));    
}

/*
 * trans_64_v2 - Transpotation for 64 * 64 matrix, version 2 seperate blocks 
 * into 4*4 to avoid miss.
 */
char trans_64_v2_desc[] = "For 64 * 64 Matrix";
void trans_64_v2(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj; //jj: column ii: row, i,j for block
    int block = 8;
    int temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            // upper
            for (i = ii; i < ii + block / 2; i++) {
                for (j = jj; j < jj + block / 2; j++) {
                    B[j][i + block / 2] = A[i][j + block / 2];
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        temp1 = A[i][j];
                        temp2 = i;                        
                    }
                }
                if (ii == jj) {
                    B[temp2][temp2] = temp1;
                }
            }
            // lower
            for (j = jj + block / 2; j < jj + block; j++) {
                temp1 = B[j - block / 2][ii + block / 2];
                temp2 = B[j - block / 2][ii + block / 2 + 1];
                temp3 = B[j - block / 2][ii + block / 2 + 2];
                temp4 = B[j - block / 2][ii + block / 2 + 3];
                temp5 = A[ii + block / 2][j - block / 2];
                temp6 = A[ii + block / 2 + 1][j - block / 2];
                temp7 = A[ii + block / 2 + 2][j - block / 2];
                temp8 = A[ii + block / 2 + 3][j - block / 2];
                B[j - block / 2][ii + block / 2] = temp5;
                B[j - block / 2][ii + block / 2 + 1] = temp6;
                B[j - block / 2][ii + block / 2 + 2] = temp7;
                B[j - block / 2][ii + block / 2 + 3] = temp8;
                B[j][ii] = temp1;
                B[j][ii + 1] = temp2;
                B[j][ii + 2] = temp3;
                B[j][ii + 3] = temp4;
            }
            for (i = ii + block / 2; i < ii + block; i++) {    
                for (j = jj + block / 2; j < jj + block; j++) {
                    if (i != j) {                       
                        B[j][i] = A[i][j];
                    } else {
                        temp1 = A[i][j];
                        temp2 = i;
                    }
                }
                if (ii == jj) {
                    B[temp2][temp2] = temp1;
                }
            }
        }
    }
    ENSURES(is_transpose(M, N, A, B));    
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
    registerTransFunction(trans_32, trans_32_desc);
    registerTransFunction(trans_64, trans_64_desc);
    registerTransFunction(trans_61_67, trans_61_67_desc);
    registerTransFunction(trans_64_v2, trans_64_v2_desc); 

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

