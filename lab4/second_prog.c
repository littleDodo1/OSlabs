#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef int (*gcd_func)(int, int);
typedef float (*pi_func)(int);

static int parse_int(const char *s) {
    int i = 0, val = 0, sign = 1;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (s[i] == '-') { sign = -1; i++; }
    else if (s[i] == '+') { i++; }
    while (s[i] >= '0' && s[i] <= '9') {
        val = val * 10 + (s[i] - '0');
        i++;
    }
    return sign * val;
}

static void parse_two_ints(const char *s, int *a, int *b) {
    const char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    *a = parse_int(p);
    while (*p >= '0' && *p <= '9') p++;
    while (*p == ' ' || *p == '\t') p++;
    *b = parse_int(p);
}

static int read_line(char *buf, int maxlen) {
    int i = 0;
    char c;
    while (i < maxlen - 1) {
        if (read(STDIN_FILENO, &c, 1) <= 0) break;
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

int main() {
    char input[128], buf[256];
    gcd_func gcd_f;
    pi_func pi_f;

    void *lib = dlopen("./libcustomlib.so", RTLD_LAZY);
    if (!lib) {
        const char msg[] = "error: cannot load libcustomlib.so\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    gcd_f = (gcd_func)dlsym(lib, "Euclids");
    pi_f = (pi_func)dlsym(lib, "row_pi");

    if (!gcd_f || !pi_f) {
        const char msg[] = "error: missing functions\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        dlclose(lib);
        return 1;
    }

    int flag = 1;

    while (1) {
        const char prompt[] = "Введите вариант (1 - gcd, 2 - pi, 0 - переключение, q - выход): ";
        write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);

        if (read_line(input, sizeof(input)) <= 0) break;
        if (input[0] == 'q' || input[0] == 'Q') break;

        int variant = parse_int(input);

        if (variant == 1) {
            write(STDOUT_FILENO, "Введите a b: ", 21);
            read_line(input, sizeof(input));

            int a, b;
            parse_two_ints(input, &a, &b);

            int res = gcd_f(a, b);
            int len = snprintf(buf, sizeof(buf), "gcd(%d, %d) = %d\n", a, b, res);
            write(STDOUT_FILENO, buf, len);
        }
        else if (variant == 2) {
            write(STDOUT_FILENO, "Введите k: ", 19);
            read_line(input, sizeof(input));

            int k = parse_int(input);
            float res = pi_f(k);

            int len = snprintf(buf, sizeof(buf), "pi ≈ %.10f (k=%d)\n", res, k);
            write(STDOUT_FILENO, buf, len);
        }
        else if (variant == 0) {
            if (flag) {
                gcd_f = (gcd_func)dlsym(lib, "naive");
                pi_f = (pi_func)dlsym(lib, "Wallis_pi");

                const char msg[] = "Переключено на naive и Wallis_pi\n";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            } else {
                gcd_f = (gcd_func)dlsym(lib, "Euclids");
                pi_f = (pi_func)dlsym(lib, "row_pi");

                const char msg[] = "Переключено на Euclids и row_pi\n";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            }

            if (!gcd_f || !pi_f) {
                const char msg[] = "Что-то не то\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                dlclose(lib);

                return 1;
            }

            flag = 1 - flag;
        }
    }

    if (lib) dlclose(lib);
    return 0;
}