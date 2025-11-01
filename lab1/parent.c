#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 512
#define MAX_LINES 100

int main() {
    char file[MAX_INPUT];
    char input_buffer[MAX_INPUT * MAX_LINES];
    char *lines[MAX_LINES];
    int line_count = 0;

    ssize_t file_len = read(STDIN_FILENO, file, sizeof(file) - 1);
    if (file_len <= 0) {
        const char msg[] = "Ошибка: не удалось прочитать имя файла\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    file[file_len] = '\0';
    
    char *newline_pos = strchr(file, '\n');
    if (newline_pos) *newline_pos = '\0';

    ssize_t total_read = 0;
    ssize_t n;
    while (total_read < (ssize_t)sizeof(input_buffer) - 1 &&
           (n = read(STDIN_FILENO, input_buffer + total_read, sizeof(input_buffer) - 1 - total_read)) > 0) {
        total_read += n;
    }
    input_buffer[total_read] = '\0';

    char *saveptr;
    char *line = strtok_r(input_buffer, "\n", &saveptr);
    while (line != NULL && line_count < MAX_LINES) {
        if (strlen(line) > 0) {
            lines[line_count++] = line;
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (line_count == 0) {
        const char msg[] = "Ошибка: нет данных для обработки\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char msg[] = "Ошибка: каналы не открыты\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid) {
        case -1: {
            const char msg[] = "Ошибка: fork не был создан\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        
        case 0:
            close(pipe1[1]);
            close(pipe2[0]);

            dup2(pipe1[0], STDIN_FILENO);
            dup2(pipe2[1], STDERR_FILENO);

            close(pipe1[0]);
            close(pipe2[1]);

            char *const args[] = {"./child", file, NULL};
            
            if (execv("./child", args) == -1) {
                const char msg[] = "Ошибка: exec\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            break;
        
        default:
            close(pipe1[0]);
            close(pipe2[1]);

            for (int i = 0; i < line_count; i++) {
                char line_with_newline[MAX_INPUT + 2];
                int len = strlen(lines[i]);
                memcpy(line_with_newline, lines[i], len);
                line_with_newline[len] = '\n';
                line_with_newline[len + 1] = '\0';
                
                if (write(pipe1[1], line_with_newline, len + 1) == -1) {
                    const char msg[] = "Ошибка: запись в дочерний процесс не выполнена\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    exit(EXIT_FAILURE);
                }
            }

            close(pipe1[1]);

            char error_buf[512];
            ssize_t error_bytes;

            while ((error_bytes = read(pipe2[0], error_buf, sizeof(error_buf) - 1)) > 0) {
                write(STDERR_FILENO, error_buf, error_bytes);
            }   

            close(pipe2[0]);

            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                exit(EXIT_FAILURE);
            }
            break;
    }

    return 0;
}