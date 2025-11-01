#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_INPUT 512

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char msg[] = "Ошибка ввода аргументов\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int output_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (output_fd == -1) {
        const char msg[] = "Ошибка: не удалось открыть файл вывода\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char line[MAX_INPUT];
    ssize_t bytes_read;
    
    while ((bytes_read = read(STDIN_FILENO, line, sizeof(line) - 1)) > 0) {
        line[bytes_read] = '\0';
        
        char *line_ptr = line;
        char *newline_pos;
        
        while ((newline_pos = strchr(line_ptr, '\n')) != NULL) {
            *newline_pos = '\0';

            if (line_ptr == newline_pos) {
                line_ptr = newline_pos + 1;
                continue;
            }

            int numbers[512];
            int count = 0;
            char *token = strtok(line_ptr, " ");
            
            while (token != NULL && count < 512) {
                int num = 0;
                int sign = 1;
                char *p = token;
                
                if (*p == '-') {
                    sign = -1;
                    p++;
                }
                
                while (*p >= '0' && *p <= '9') {
                    num = num * 10 + (*p - '0');
                    p++;
                }
                
                numbers[count++] = num * sign;
                token = strtok(NULL, " ");
            }

            if (count < 2) {
                const char msg[] = "Ошибка: недостаточно чисел в строке\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                close(output_fd);
                exit(EXIT_FAILURE);
            }

            int dividend = numbers[0];
            
            char output_buf[MAX_INPUT * 2] = {0};
            char num_buf[32];
            int output_len = 0;
            
            const char prefix[] = "Деление ";
            memcpy(output_buf, prefix, sizeof(prefix) - 1);
            output_len += sizeof(prefix) - 1;
            
            char *p = num_buf;
            int n = dividend;
            if (n < 0) {
                output_buf[output_len++] = '-';
                n = -n;
            }
            int div = 1;
            while (n / div > 9) div *= 10;
            while (div > 0) {
                output_buf[output_len++] = '0' + n / div;
                n %= div;
                div /= 10;
            }
            if (dividend == 0) output_buf[output_len++] = '0';
            
            const char suffix[] = " на: ";
            memcpy(output_buf + output_len, suffix, sizeof(suffix) - 1);
            output_len += sizeof(suffix) - 1;

            for (int i = 1; i < count; i++) {
                int divisor = numbers[i];
                
                if (divisor == 0) {
                    const char msg[] = "Ошибка: деление на ноль\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    close(output_fd);
                    exit(EXIT_FAILURE);
                }
                
                if (i > 1) {
                    output_buf[output_len++] = ',';
                    output_buf[output_len++] = ' ';
                }
                
                n = dividend;
                if (n < 0) {
                    output_buf[output_len++] = '-';
                    n = -n;
                }
                div = 1;
                while (n / div > 9) div *= 10;
                while (div > 0) {
                    output_buf[output_len++] = '0' + n / div;
                    n %= div;
                    div /= 10;
                }
                if (dividend == 0) output_buf[output_len++] = '0';
                
                output_buf[output_len++] = '/';
                
                n = divisor;
                if (n < 0) {
                    output_buf[output_len++] = '-';
                    n = -n;
                }
                div = 1;
                while (n / div > 9) div *= 10;
                while (div > 0) {
                    output_buf[output_len++] = '0' + n / div;
                    n %= div;
                    div /= 10;
                }
                if (divisor == 0) output_buf[output_len++] = '0';
                
                output_buf[output_len++] = '=';
                
                float result = (float)dividend / divisor;
                
                int int_part = (int)result;
                int frac_part = (int)((result - int_part) * 100 + 0.5);
                if (frac_part < 0) frac_part = -frac_part;
                
                n = int_part;
                if (n < 0) {
                    output_buf[output_len++] = '-';
                    n = -n;
                }
                div = 1;
                while (n / div > 9) div *= 10;
                while (div > 0) {
                    output_buf[output_len++] = '0' + n / div;
                    n %= div;
                    div /= 10;
                }
                if (int_part == 0) output_buf[output_len++] = '0';
                
                output_buf[output_len++] = '.';
                output_buf[output_len++] = '0' + (frac_part / 10);
                output_buf[output_len++] = '0' + (frac_part % 10);
            }
            
            output_buf[output_len++] = '\n';
            
            write(output_fd, output_buf, output_len);
            
            line_ptr = newline_pos + 1;
        }
    }

    close(output_fd);
    return 0;
}