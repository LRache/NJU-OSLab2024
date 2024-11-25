#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/wait.h>

char functionBuffer[1 << 14]; // 4KB buffer

static char **environment;

bool is_function(const char *line) {
    const char *p = "int";
    for (int i = 0; i < 3 && line[i] != '\0'; i++) {
        if (line[i] != p[i]) {
            return false;
        }
    }
    return true;
}

bool run_command(char * const *argv) {
    pid_t pid;
    if ((pid = fork()) == 0) {
        execve(argv[0], argv, environment);
        return true;
    } else {
        if (pid < 0) return false;
        int status;
        pid_t p = waitpid(pid, &status, 0);
        if (p == -1) return false;
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status) == 0;   
        } else {
            return false;
        }
    }
}

bool do_syntax_check(const char *line) {
    char tempFilePath[] = "/tmp/crepl_expr_XXXXXX.c";
    int fd = mkstemps(tempFilePath, 2);
    FILE *tempFile = fdopen(fd, "w");
    fprintf(tempFile, "%s\n%s", functionBuffer, line);
    fclose(tempFile);
    
    const char *argv[4];
    argv[0] = "/usr/bin/gcc";
    argv[1] = tempFilePath;
    argv[2] = "-fsyntax-only";
    argv[3] = NULL;

    if (run_command((char * const *)argv)) {
        unlink(tempFilePath);
        return true;
    } else {
        unlink(tempFilePath);
        return false;
    }
}

bool compile_file(const char *filename, const char *outputFilename) {
    const char *argv[7];
    argv[0] = "/usr/bin/gcc";
    argv[1] = filename;
    argv[2] = "-o";
    argv[3] = outputFilename;
    argv[4] = "-shared";
    argv[5] = "-fPIC";
    argv[6] = NULL;
    return run_command((char * const *)argv);
}

bool run_expr(const char *line) {
    char buffer[4096 + 1024];
    sprintf(buffer, "int __expr_wrapper() { return %s; }", line);

    char tempFilePath[] = "/tmp/crepl_expr_XXXXXX.c";
    int fd = mkstemps(tempFilePath, 2);
    FILE *tempFile = fdopen(fd, "w");
    fprintf(tempFile, "%s\n%s", functionBuffer, buffer);
    fclose(tempFile);
    if (!compile_file(tempFilePath, "/tmp/crepl.so")) {
        fprintf(stderr, "Error when compile shared libary.\n");
        unlink(tempFilePath);
        return false;
    }
    unlink(tempFilePath);

    void *handle = dlopen("/tmp/crepl.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return false;
    }
    dlerror();

    int (*wrapper)() = dlsym(handle, "__expr_wrapper");
    char *errorMsg;
    if ((errorMsg = dlerror()) != NULL) {
        fprintf(stderr, "Error: %s\n", errorMsg);
        dlclose(handle);
        return false;
    }

    printf("(%s) == %d\n", line, wrapper());
    dlclose(handle);

    return true;
}


int main(int argc, char *argv[], char *envp[]) {
    environment = envp;
    functionBuffer[0] = 0;
    
    static char line[4096];
    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        line[strlen(line) - 1] = 0;

        if (is_function(line)) {
            if (do_syntax_check(line)) {
                printf("OK.\n");
                sprintf(functionBuffer, "%s\n%s", functionBuffer, line);
            } else {
                printf("Sytax Error.\n");
            }
        } else {
            run_expr(line);
        }
    }

    return 0;
}
