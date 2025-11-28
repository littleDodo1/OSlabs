#include "../include/lib1.h"

int Euclids(int a, int b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

float row_pi(int k) {
    if (k < 0) return 0.0;

    float res = 0;
    for(int i = 0; i < k; i++) {
        float term = 1.0 / (2 * i + 1);
        if (i % 2 == 1) term = -term;
        res += term;
    }

    return 4.0 * res;
}

