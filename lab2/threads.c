#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

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

void merge(int *arr, int l, int m, int r, int *temp) {
    int n1 = m - l + 1;
    int n2 = r - m;
    memcpy(temp, arr + l, n1 * sizeof(int));
    memcpy(temp + n1, arr + m + 1, n2 * sizeof(int));
    int i = 0, j = n1, k = l;
    while (i < n1 && j < n1 + n2) {
        if (temp[i] <= temp[j]) {
            arr[k++] = temp[i++];
        } else {
            arr[k++] = temp[j++];
        }
    }
    while (i < n1) arr[k++] = temp[i++];
    while (j < n1 + n2) arr[k++] = temp[j++];
}

int calc_minrun(int n) {
    int r = 0;
    while (n >= 512) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

typedef struct {
    int *arr;
    int left;
    int right;
} SortTask;

static void *sort_run(void *arg) {
    SortTask *task = (SortTask *)arg;
    insertion_sort(task->arr, task->left, task->right);
    return NULL;
}

void timsort(int *arr, int n, int max_threads) {
    if (n <= 1) return;

    int minrun = (n > 512) ? calc_minrun(n) : 64;
    int max_blocks = (n + minrun - 1) / minrun;
    SortTask *tasks = malloc(max_blocks * sizeof(SortTask));
    if (!tasks) {
        const char msg[] = "Ошибка: ошибка выделения памяти tasks\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int task_count = 0;
    for (int i = 0; i < n; i += minrun) {
        int end = (i + minrun <= n) ? i + minrun - 1 : n - 1;
        tasks[task_count++] = (SortTask){ .arr = arr, .left = i, .right = end };
    }

    int *temp_buffer = malloc(n * sizeof(int));
    if (!temp_buffer) {
        const char msg[] = "Ошибка: ошибка выделения памяти temp_buffer\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    if (max_threads > 1) {
        for (int j = 0; j < task_count; j += max_threads) {
            int size = (j + max_threads < task_count) ? max_threads : (task_count - j);
            pthread_t *threads = malloc(size * sizeof(pthread_t));
            if (!threads) {
                const char msg[] = "Ошибка: ошибка выделения памяти threads\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            for (int t = 0; t < size; t++) {
                pthread_create(&threads[t], NULL, sort_run, &tasks[j + t]);
            }
            for (int t = 0; t < size; t++) {
                pthread_join(threads[t], NULL);
            }
            free(threads);
        }
    } else {
        for (int k = 0; k < task_count; k++) {
            insertion_sort(arr, tasks[k].left, tasks[k].right);
        }
    }

    // for (int j = 0; j < task_count; j += max_threads) {
    //         int size = (j + max_threads < task_count) ? max_threads : (task_count - j);
    //         pthread_t *threads = malloc(size * sizeof(pthread_t));

    //         if (!threads) {
    //             const char msg[] = "Ошибка: ошибка выделения памяти threads\n";
    //             write(STDERR_FILENO, msg, sizeof(msg) - 1);
    //             exit(EXIT_FAILURE);
    //         }

    //         for (int t = 0; t < size; t++) {
    //             pthread_create(&threads[t], NULL, sort_run, &tasks[j + t]);
    //         }

    //         for (int t = 0; t < size; t++) {
    //             pthread_join(threads[t], NULL);
    //         }

    //         free(threads);
    // }

    int width = minrun;
    while (width < n) {
        for (int left = 0; left < n; left += 2 * width) {
            int mid = left + width;
            if (mid >= n) break;
            int right = (left + 2 * width <= n) ? left + 2 * width : n;
            merge(arr, left, mid - 1, right - 1, temp_buffer);
        }
        width *= 2;
    }

    free(temp_buffer);
    free(tasks);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        const char msg[] = "Ошибка: некорректный ввод\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    int n = atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    int *arr = malloc(n * sizeof(int));
    if (!arr) {
        const char msg[] = "Ошибка: ошибка выделения памяти\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 10000;
    }

    if (n < 100) {     
        const char out1[] = "Массив чисел до сортировки: ";
        write(STDOUT_FILENO, out1, sizeof(out1) - 1);
        for (int i = 0; i < n; i++) {
            print_number(arr[i]);
            write(STDOUT_FILENO, " ", 1);
        }
        write(STDOUT_FILENO, "\n", 1);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    timsort(arr, n, max_threads);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (n < 100) {
        const char out2[] = "Массив чисел после сортировки: ";
        write(STDOUT_FILENO, out2, sizeof(out2) - 1);
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