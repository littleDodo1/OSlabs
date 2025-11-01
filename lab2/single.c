#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <string.h>
#include <limits.h>


void print_number(int num) {
    char buffer[32];  
    int len = snprintf(buffer, sizeof(buffer), "%d", num);
    write(STDOUT_FILENO, buffer, len);
}

void insertion_sort(int *arr, int left, int right) {
    for (int i = left + 1; i <= right; ++i) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            --j;
        }
        arr[j + 1] = key;
    }
}

void merge(int *arr, int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    memcpy(L, arr + l, n1 * sizeof(int));
    memcpy(R, arr + m + 1, n2 * sizeof(int));

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

int calc_minrun(int n) {
    int r = 0;
    while (n >= 32) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

void timsort(int *arr, int n) {
    if (n <= 1) return;

    int minrun = calc_minrun(n);
    int stack_start[256];
    int stack_len[256];
    int stack_size = 0;

    int i = 0;
    while (i < n) {
        int start = i;
        int end = i + 1;

        if (end < n && arr[end] < arr[start]) {
            while (end < n - 1 && arr[end + 1] <= arr[end]) {
                ++end;
            }
            int l = start, r = end;
            while (l < r) {
                int tmp = arr[l];
                arr[l] = arr[r];
                arr[r] = tmp;
                ++l; --r;
            }
        } else {
            while (end < n - 1 && arr[end + 1] >= arr[end]) {
                ++end;
            }
        }

        int run_len = end - start + 1;

        if (run_len < minrun) {
            int forced_end = (start + minrun < n) ? start + minrun : n - 1;
            insertion_sort(arr, start, forced_end);
            run_len = forced_end - start + 1;
        }

        stack_start[stack_size] = start;
        stack_len[stack_size] = run_len;
        stack_size++;
        i = start + run_len;

        while (1) {
            if (stack_size >= 3) {
                int a = stack_size - 3;
                int b = stack_size - 2;
                int c = stack_size - 1;

                if (stack_len[a] <= stack_len[b] + stack_len[c] ||
                    stack_len[b] <= stack_len[c]) {

                    if (stack_len[a] <= stack_len[c]) {
                        int start1 = stack_start[a];
                        int len1 = stack_len[a];
                        int len2 = stack_len[b];
                        merge(arr, start1, start1 + len1 - 1, start1 + len1 + len2 - 1);
                        
                        stack_len[a] += stack_len[b];
                        stack_start[b] = stack_start[c];
                        stack_len[b] = stack_len[c];
                        stack_size--;
                    } else {
                        int start1 = stack_start[b];
                        int len1 = stack_len[b];
                        int len2 = stack_len[c];
                        merge(arr, start1, start1 + len1 - 1, start1 + len1 + len2 - 1);
                        stack_len[b] += stack_len[c];
                        stack_size--;
                    }

                    continue;
                }
            }

            if (stack_size >= 2) {
                int b = stack_size - 2;
                int c = stack_size - 1;
                if (stack_len[b] <= stack_len[c]) {
                    int start1 = stack_start[b];
                    int len1 = stack_len[b];
                    int len2 = stack_len[c];
                    merge(arr, start1, start1 + len1 - 1, start1 + len1 + len2 - 1);
                    stack_len[b] += stack_len[c];
                    stack_size--;
                    continue;
                }
            }

            break;
        }
    }

    while (stack_size > 1) {
        int b = stack_size - 2;
        int c = stack_size - 1;
        int start1 = stack_start[b];
        int len1 = stack_len[b];
        int len2 = stack_len[c];
        merge(arr, start1, start1 + len1 - 1, start1 + len1 + len2 - 1);
        stack_len[b] += stack_len[c];
        stack_size--;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        const char msg[] = "Ошибка: некорректный ввод\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    int n = atoi(argv[1]);
    int *arr = (int *)malloc(n * sizeof(int));

    if (!arr) {
        const char msg[] = "Ошибка: ошибка выделения памяти\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 1000; 
    }

    if (n < 100) {
        const char out1[] = "Массив чисел до сортировки: ";
        write(STDOUT_FILENO, out1, sizeof(out1));

        for (int i = 0; i < n; i++) {
            print_number(arr[i]);
            write(STDOUT_FILENO, " ", 1);
        }

        write(STDOUT_FILENO, "\n", 1);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    timsort(arr, n);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (n < 100){
        const char out2[] = "Массив чисел после сортировки: ";
        write(STDOUT_FILENO, out2, sizeof(out2));

        for (int i = 0; i < n; i++) {
            print_number(arr[i]);
            write(STDOUT_FILENO, " ", 1);
        }

        write(STDOUT_FILENO, "\n", 1);
    }
    
    double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    char msg[100];
    int len = snprintf(msg, sizeof(msg), "Время: %.8f сек\n", time);
    write(STDOUT_FILENO, msg, len);
    write(STDOUT_FILENO, "\n", 1);

    free(arr);
    return EXIT_SUCCESS;
}