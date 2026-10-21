#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "include/cMatrix.h"

void free_all(cmatrix_float* mtrzs[], int n){
    if(!mtrzs) return;
    for(int i = 0; i < n; i++){
        if(mtrzs[i] != NULL){
            cM(mtrzs[i], free); 
            mtrzs[i] = NULL;
        }
    }
}

typedef enum{
    BASIC_SOLUTION = 0,
    BASIC_FEASIBLE_SOLUTION
} PPLSolution_t;

int64_t next_combination(int64_t x){
    int64_t c = x & -x;
    int64_t r = x + c;
    return (((x^r) >> 2) / c) | r;
}

size_t comb(int n, int m){
    if(n < m) return 0;
    float c = 1;
    for(int i = 0; i < m; i++){
        c *= ((float)(n - i)) / (m - i);
    }
    return  roundf(c);
}

void print_bits(long x, int n){
    for(int i = n - 1; i >= 0; i--){
        printf("%lld", (x >> i) & 1ll);
    }
    putchar('\n');
}

float diag_mult(cmatrix_float* A){
    float diag_det = 1;
    for(size_t i = 0; i < A->rows; i++){
        size_t idx = i*A->cols + i;
        diag_det *= A->data[idx];
    }
    return diag_det;
}

cmatrix_error generate_all_BS(cmatrix_float*** solutions, size_t* n_solutions, cmatrix_float* Ab, PPLSolution_t bfs){
    if(!Ab) return CMATRIX_NULL_POINTER;
    size_t n = Ab->cols - 1, m = Ab->rows;
  
    cmatrix_float* b = new_cmatrix_float(Ab->rows, 1);
    cM(b, getCol, Ab, n);
    cmatrix_float* v[n];

    for(int i = 0; i < n; i++) {
        v[i] = new_cmatrix_float(Ab->rows, 1);
        cM(v[i], getCol, Ab, i);
    }

    size_t n_combs = comb(n, m);
    *solutions = malloc(n_combs * sizeof(cmatrix_float*));
    for(size_t i = 0; i < n_combs; i++){
        (*solutions)[i] = new_cmatrix_float(m, 1);
    }
    if(!*solutions) return CMATRIX_ALLOC_ERROR;

    int64_t mask = (1LL << m) - 1;
    
    cmatrix_float* Bb = new_cmatrix_float(m, m + 1);
    cmatrix_float* x = new_cmatrix_float(n, 1);
    int cntbasics = 0;
    for(int i = 0; i < n_combs; i++){
        cM(Bb, setCol, b, m);
        int cnt = 0;
        for(int j = 0; j < n; j++){
            if((mask >> j) & 1)    
                cM(Bb, setCol, v[j], cnt++);
        }

        cM(Bb, rref, Bb);
        if(fabs(diag_mult(Bb)) > 1e-6){
            int top = 0;
            for(int j = 0; j < n; j++){
                x->data[j] = ((mask >> j) & 1) ? Bb->data[top*Bb->cols + m] : 0;
                top += (mask >> j) & 1;
            }
            if(!bfs || cM(x, atLeast, 0)){
                cM((*solutions)[cntbasics], cpy, x);
                cntbasics++;
            }
        }
        mask = next_combination(mask);
    }
    if(cntbasics == 0) return CMATRIX_SINGULAR_MATRIX;
       *solutions = realloc(*solutions, cntbasics * sizeof(cmatrix_float*));
    *n_solutions = cntbasics;

    cM(Bb, free);
    cM(b, free);
    for(size_t i = 0; i < n; i++) cM(v[i], free);
    return CMATRIX_OK;
}

int setargs(int argc, const char* argv[], PPLSolution_t* vbn){
    if(argc < 2){
        printf(
            "chamada sem argumentos. use:\n"
            "-sb para gerar as soluções basicas\n"
            "-sbv para gerar as básicas viáveis\n"
        );
        return 0;
    }
    if(strcmp(argv[1], "-sbv") == 0){
        *vbn = BASIC_FEASIBLE_SOLUTION;
    }else if(strcmp(argv[1], "-sb") == 0){
        *vbn = BASIC_SOLUTION;
    }else{
        printf(
            "argumentos invalidos. use:\n"
            "-sb para gerar as soluções basicas\n"
            "-sbv para gerar as básicas viáveis\n"
        );
        return 0;
    }
    return 1;
}

int main(int argc, const char* argv[]){
    PPLSolution_t vbn;
    if(!setargs(argc, argv, &vbn)) goto fim;
    
    size_t m, n;
    scanf(" %zu %zu", &m, &n);
    cmatrix_float* Ab = get_cmatrix_float(m, n); 
    cmatrix_float** x = NULL;
    size_t n_x;
    cmatrix_error err = generate_all_BS(&x, &n_x, Ab, vbn);
    if(err == CMATRIX_SINGULAR_MATRIX){
        printf("Nenhuma solução basica viável\n");
        goto fim;
    }

    for(int i = 0; i < n_x; i++)
        cM(x[i], print);
    
    if(vbn)
        printf("numero de soluções básicas viáveis: %zu\n", n_x);
    else
        printf("numero de soluções básicas: %zu\n", n_x);

    cM(Ab, free);
    for(int i = 0; i < n_x; i++) cM(x[i], free);
    free(x);
fim:
    return 0;
}
