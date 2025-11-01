#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdio.h>

#define SHM_SIZE 4096
#define MAX_LINES 100

int main() {
    pid_t my_pid = getpid();
    char shm_name[64], sem_name[64];
    snprintf(shm_name, sizeof(shm_name), "/shm_%d", (int)my_pid);
    snprintf(sem_name, sizeof(sem_name), "/sem_%d", (int)my_pid);

    char filename[256];
    ssize_t len = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (len <= 0) {
        const char msg[] = "Ошибка: не удалось прочитать имя файла\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    filename[len] = '\0';
    char *nl = strchr(filename, '\n');
    if (nl) *nl = '\0';

    char input[SHM_SIZE - sizeof(uint32_t)];
    ssize_t total = 0, n;
    while (total < (ssize_t)sizeof(input) &&
           (n = read(STDIN_FILENO, input + total, sizeof(input) - total)) > 0) {
        total += n;
    }
    input[total] = '\0';

    char *lines[MAX_LINES];
    int count = 0;
    char *saveptr, *line = strtok_r(input, "\n", &saveptr);
    while (line && count < MAX_LINES) {
        if (strlen(line) > 0) lines[count++] = line;
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (count == 0) {
        const char msg[] = "Ошибка: нет данных\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (shm_fd == -1) {
        const char msg[] = "Ошибка: shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        const char msg[] = "Ошибка: ftruncate\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        const char msg[] = "Ошибка: mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem == SEM_FAILED) {
        const char msg[] = "Ошибка: sem_open\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    uint32_t *p_count = (uint32_t *)shm;
    char *data = shm + sizeof(uint32_t);
    *p_count = count;

    size_t offset = 0;
    for (int i = 0; i < count; i++) {
        size_t l = strlen(lines[i]) + 1;
        if (offset + l > SHM_SIZE - sizeof(uint32_t)) break;
        memcpy(data + offset, lines[i], l);
        offset += l;
    }

    pid_t child = fork();
    if (child == 0) {
        char pid_str[32];
        snprintf(pid_str, sizeof(pid_str), "%d", (int)getppid());
        char *args[] = {"./child", filename, pid_str, NULL};
        
        execv("./child", args);

        const char msg[] = "Ошибка: execv\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    } else if (child == -1) {
        const char msg[] = "Ошибка: fork\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_post(sem);

    int status;
    waitpid(child, &status, 0);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            const char msg[] = "Ошибка: дочерний процесс завершился с ошибкой\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    sem_close(sem);
    sem_unlink(sem_name);
    munmap(shm, SHM_SIZE);
    close(shm_fd);
    shm_unlink(shm_name);

    return 0;
}