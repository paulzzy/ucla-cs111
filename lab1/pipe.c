#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    return -EINVAL;
  }

  int file_descriptors[2];
  int writer_pid = 0;
  int reader_pid = 0;

  if (pipe(file_descriptors) == -1) {
    return -1;
  }

  int pipe_read_fd = file_descriptors[0];
  int pipe_write_fd = file_descriptors[1];

  // Child that writes
  writer_pid = fork();
  if (writer_pid == 0) {
    // `stdout` is written to pipe
    dup2(pipe_write_fd, STDOUT_FILENO);
    close(pipe_read_fd);
    execlp(argv[1], argv[1], NULL);
  } else if (writer_pid == -1) {
    return -1;
  }

  // Child that reads
  reader_pid = fork();
  if (reader_pid == 0) {
    // `stdin` is read from pipe
    dup2(pipe_read_fd, STDIN_FILENO);
    close(pipe_write_fd);
    execlp(argv[2], argv[2], NULL);
  } else if (writer_pid == -1) {
    return -1;
  }

  close(pipe_read_fd);
  close(pipe_write_fd);

  waitpid(writer_pid, NULL, 0);
  waitpid(reader_pid, NULL, 0);

  return 0;
}
