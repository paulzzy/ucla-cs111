#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

enum PipeAction { READ = 0, WRITE = 1 };

// Short for "debug print pipe" because I don't like typing
void dpp(int* pipe) {
  // Cast to void to silence cert-err33-c clang-tidy warning
  (void)fprintf(stderr, "pipe: read %d, write %d\n", pipe[READ], pipe[WRITE]);
}

void copy_pipe(const int* source, int* dest) {
  dest[READ] = source[READ];
  dest[WRITE] = source[WRITE];
}

void duplicate_fd_wrapper(int* pipe, enum PipeAction action) {
  int pipe_fd = (action == READ) ? pipe[READ] : pipe[WRITE];
  int stdio_fd = (action == READ) ? STDIN_FILENO : STDOUT_FILENO;

  if (dup2(pipe_fd, stdio_fd) == -1) {
    exit(errno);
  }

  close(pipe[READ]);
  close(pipe[WRITE]);
}

int wait_wrapper(int child_pid) {
  int child_status = 0;

  if (waitpid(child_pid, &child_status, 0) == -1) {
    exit(errno);
  };

  if (WIFEXITED(child_status)) {
    return WEXITSTATUS(child_status);
  }

  exit(ECHILD);
}

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    // As required by spec
    return EINVAL;
  }

  if (argc == 2) {
    if (execlp(argv[1], argv[1], NULL) == -1) {
      return errno;
    }

    return EXIT_SUCCESS;
  }

  int prev_pipe[2] = {0};
  int current_pipe[2] = {0};

  for (int i = 1; i < argc; i++) {
    if (pipe(current_pipe) == -1) {
      return errno;
    }

    int child_pid = fork();
    switch (child_pid) {
      case -1:
        return errno;

      case 0:
        // `stdin` is read from pipe, except for the first command. That
        // reads directly from `stdin`.
        //
        // Similarly, `stdout` is written to pipe, except for the last
        // command. That writes directly to `stdout`.
        if (i == 1) {
          // `stdin` is not modified. Only `stdout` is redirected.
          duplicate_fd_wrapper(current_pipe, WRITE);
        } else if (i == argc - 1) {
          // `stdout` is not modified. Only `stdin` is redirected.
          duplicate_fd_wrapper(prev_pipe, READ);
        } else {
          duplicate_fd_wrapper(prev_pipe, READ);
          // Writes go to a different pipe
          duplicate_fd_wrapper(current_pipe, WRITE);
        }

        if (execlp(argv[i], argv[i], NULL) == -1) {
          return errno;
        };
        break;

      default:
        close(current_pipe[WRITE]);

        int child_status = wait_wrapper(child_pid);
        if (child_status != EXIT_SUCCESS) {
          return child_status;
        }
        break;
    }

    copy_pipe(current_pipe, prev_pipe);
  }

  return EXIT_SUCCESS;
}
