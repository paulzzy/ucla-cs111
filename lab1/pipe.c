#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

struct PipeWrapper {
  int fds[2];
  int read_fd;
  int write_fd;
};

enum PipeAction { PIPE_READ, PIPE_WRITE };

int duplicate_fd_wrapper(struct PipeWrapper* pipe, enum PipeAction action) {
  int pipe_fd = (action == PIPE_READ) ? pipe->read_fd : pipe->write_fd;
  int stdio_fd = (action == PIPE_READ) ? STDIN_FILENO : STDOUT_FILENO;

  if (dup2(pipe_fd, stdio_fd) == -1) {
    exit(errno);
  }

  if (close(pipe->read_fd) == -1 || close(pipe->write_fd) == -1) {
    exit(errno);
  };

  return 0;
}

int wait_wrapper(int child_pid) {
  int child_status = 0;

  if (waitpid(child_pid, &child_status, 0) == -1) {
    exit(errno);
  };

  if (WIFEXITED(child_status)) {
    printf("yay waited for %d with exit %d\n", child_pid,
           WEXITSTATUS(child_status));
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

  struct PipeWrapper first_pipe;
  struct PipeWrapper second_pipe;
  struct PipeWrapper* current_pipe = &first_pipe;

  if ((pipe(first_pipe.fds) == -1) || (pipe(second_pipe.fds) == -1)) {
    return errno;
  }

  first_pipe.read_fd = first_pipe.fds[0];
  first_pipe.write_fd = first_pipe.fds[1];
  second_pipe.read_fd = second_pipe.fds[0];
  second_pipe.write_fd = second_pipe.fds[1];

  for (int i = 1; i < argc; i++) {
    int child_pid = fork();
    switch (child_pid) {
      case -1:
        return errno;

      case 0:
        // `stdin` is read from pipe, except for the first command. That reads
        // directly from `stdin`.
        //
        // Similarly, `stdout` is written to pipe, except for the last command.
        // That writes directly to `stdout`.
        if (i == 1) {
          // `stdin` is not modified. Only `stdout` is redirected.
          duplicate_fd_wrapper(current_pipe, PIPE_WRITE);
        } else if (i == argc - 1) {
          // `stdout` is not modified. Only `stdin` is redirected.
          duplicate_fd_wrapper(current_pipe, PIPE_READ);
        } else {
          duplicate_fd_wrapper(current_pipe, PIPE_READ);
          // Switch current pipe so writes go to a different pipe
          current_pipe = current_pipe->write_fd == first_pipe.write_fd
                             ? &second_pipe
                             : &first_pipe;
          duplicate_fd_wrapper(current_pipe, PIPE_WRITE);
        }

        if (execlp(argv[i], argv[i], NULL) == -1) {
          return errno;
        };
        break;

      default:
        // printf("boutta wait for %d\n", child_pid);
        // close(current_pipe->write_fd);
        // wait_wrapper(child_pid);
        break;
    }
  }

  close(first_pipe.read_fd);
  close(first_pipe.write_fd);
  close(second_pipe.read_fd);
  close(second_pipe.write_fd);

  return EXIT_SUCCESS;
}
