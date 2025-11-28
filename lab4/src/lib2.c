#include "../include/lib2.h"

int naive(int a, int b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;

    int min = (a < b) ? a : b;
    for (int i = min; i >= 1; --i) {
        if (a % i == 0 && b % i == 0) {
            return i;
        }
    }

    return 1;
}

float Wallis_pi(int k) {
    if (k <= 0) return 0.0;

    float prod = 1.0; 
    for(int i = 1; i <= k; i++) {
        prod *= (4.0f * i * i) / ((2.0f * i - 1.0f) * (2.0f * i + 1.0f));
    }

    return 2.0 * prod;
}