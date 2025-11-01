#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

#define SHM_SIZE 4096

static int to_int(const char* str) {
    if (!str) {
        const char msg[] = "Ошибка: пустая строка\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const char* p = str;
    while (*p == ' ' || *p == '\t') p++;

    if (*p == '\0') {
        const char msg[] = "Ошибка: строка содержит только пробелы\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    if (!isdigit((unsigned char)*p)) {
        const char msg[] = "Ошибка: нечисловой символ\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    long long num = 0;
    while (*p) {
        if (!isdigit((unsigned char)*p)) {
            const char msg[] = "Ошибка: нечисловой символ\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        num = num * 10 + (*p - '0');
        if (num * sign > INT_MAX || num * sign < INT_MIN) {
            const char msg[] = "Ошибка: число выходит за пределы int\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        p++;
    }

    num *= sign;
    return (int)num;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char msg[] = "Ошибка: неверное количество аргументов\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int parent_pid = atoi(argv[2]);

    char shm_name[64], sem_name[64];
    snprintf(shm_name, sizeof(shm_name), "/shm_%d", parent_pid);
    snprintf(sem_name, sizeof(sem_name), "/sem_%d", parent_pid);

    sem_t *sem = sem_open(sem_name, 0);
    if (sem == SEM_FAILED) {
        const char msg[] = "Ошибка: sem_open\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (sem_wait(sem) == -1) {
        const char msg[] = "Ошибка: sem_wait\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(shm_name, O_RDWR, 0);
    if (shm_fd == -1) {
        const char msg[] = "Ошибка: shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *shm = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        const char msg[] = "Ошибка: mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    uint32_t count = *(uint32_t *)shm;
    char *data = shm + sizeof(uint32_t);

    int out_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (out_fd == -1) {
        const char msg[] = "Ошибка: не удалось открыть файл вывода\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        munmap(shm, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    char *line = data;
    for (uint32_t i = 0; i < count; i++) {
        if (*line == '\0') break;

        char line_copy[512];
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';

        int nums[512];
        int num_count = 0;
        char *token = strtok(line_copy, " ");
        
        while (token && num_count < 512) {
            nums[num_count++] = to_int(token);
            token = strtok(NULL, " ");
        }

        if (num_count < 2) {
            const char err_msg[] = "Ошибка: недостаточно чисел в строке\n";
            write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
            close(out_fd);
            munmap(shm, SHM_SIZE);
            close(shm_fd);
            sem_close(sem);
            exit(EXIT_FAILURE);
        }

        char buf[1024];
        int pos = snprintf(buf, sizeof(buf), "Деление %d на: ", nums[0]);

        for (int j = 1; j < num_count; j++) {
            if (j > 1) {
                if (pos < sizeof(buf) - 2) {
                    buf[pos++] = ',';
                    buf[pos++] = ' ';
                }
            }

            if (nums[j] == 0) {
                const char err_msg[] = "Ошибка: деление на ноль\n";
                write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
                close(out_fd);
                munmap(shm, SHM_SIZE);
                close(shm_fd);
                sem_close(sem);
                exit(EXIT_FAILURE);
            } else {
                float res = (float)nums[0] / nums[j];
                if (pos < sizeof(buf) - 50) {
                    pos += snprintf(buf + pos, sizeof(buf) - pos, 
                        "%d/%d=%.2f", nums[0], nums[j], res);
                }
            }
        }
        
        if (pos < sizeof(buf) - 1) {
            buf[pos++] = '\n';
            write(out_fd, buf, pos);
        }

        line += strlen(line) + 1;
    }

    close(out_fd);
    munmap(shm, SHM_SIZE);
    close(shm_fd);
    sem_close(sem);
    
    return 0;
}