/*******************************************************************************
 * Name        : utils.c
 * Author      : Yuvan Rengifo
 * Pledge      : I pledge my honor that I have abided by the Stevens honor system
 ******************************************************************************/
#include "utils.h"

// Comparison functions
int cmpr_int(void* a, void* b) {
    int fst = *(int*)a; // dereferenced
    int snd = *(int*)b; // dereferenced

    if (fst > snd) {
        return 1;
    } else if (fst < snd) {
        return -1;
    } else {
        return 0;
    }
}

int cmpr_float(void* a, void* b) {
    float fst = *(float*)a; // dereferenced
    float snd = *(float*)b; // dereferenced

    if (fst > snd) {
        return 1;
    } else if (fst < snd) {
        return -1;
    } else {
        return 0;
    }
}

// Functions to print
void print_int(void* a) {
    printf("%d ", *(int*)a);  // print derederenced int
}
void print_float(void* a) {
    printf("%f ", *(float*)a); //print dereferenced float
}

