#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process {
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

static int compare_by_arrival(const void *left, const void *right) {
  u32 diff = ((struct process *)left)->arrival_time -
             ((struct process *)right)->arrival_time;

  // Stable sort by returning `left` less than `right` when they have equal
  // arrival times, assuming that the array is sorted left-to-right
  return (diff != 0) ? (int)diff : -1;
}

static struct process *remove_and_get_next(struct process_list *list,
                                           struct process *current) {
  struct process *next = TAILQ_NEXT(current, pointers);
  TAILQ_REMOVE(list, current, pointers);
  // printf("curr pid: %d\n", current->pid);
  // printf("next pid: %d\n", next == NULL ? -1 : next->pid);
  return next;
}

u32 next_int(const char **data, const char *data_end) {
  u32 current = 0;
  bool started = false;
  while (*data != data_end) {
    char c = **data;

    if (c < 0x30 || c > 0x39) {
      if (started) {
        return current;
      }
    } else {
      if (!started) {
        current = (c - 0x30);
        started = true;
      } else {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data) {
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++])) {
    if (c < 0x30 || c > 0x39) {
      exit(EINVAL);
    }
    if (!started) {
      current = (c - 0x30);
      started = true;
    } else {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path, struct process **process_data,
                    u32 *process_size) {
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED) {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL) {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i) {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  if (size > 0) {
    u32 current_time = 0;
    u32 run_time = 0;

    u32 queue_idx = 0;

    struct process *current = NULL;

    // I swear to god this better be stable sort 🙏
    qsort(data, size, sizeof(struct process), compare_by_arrival);

    // Exit loop when all processes are done executing.
    //
    // "Runs" a process at the end, so all checking happens at the start
    while (queue_idx < size || (queue_idx == size && !TAILQ_EMPTY(&list))) {
      // Add new process to list if now is arrival time
      if (queue_idx < size && data[queue_idx].arrival_time == current_time) {
        TAILQ_INSERT_TAIL(&list, &data[queue_idx], pointers);
        queue_idx++;

        // Run new process if it's the only process
        if (current == NULL) {
          current = TAILQ_FIRST(&list);
        }
      }

      // Switch to next process if current is finished
      if (current->burst_time == 0) {
        current = remove_and_get_next(&list, current);
        run_time = 0;

        // Update total response time if there are more runnable processes
        // if (current != NULL) {
        //   printf("pid %d arrived: %d, now: %d\n", current->pid,
        //          current->arrival_time, current_time);
        //   total_response_time += current_time - current->arrival_time;
        // }
      }

      // Interrupt process when it uses up a quantum
      if (run_time == quantum_length) {
        struct process *timed_out_process = current;
        current = remove_and_get_next(&list, current);
        TAILQ_INSERT_TAIL(&list, timed_out_process, pointers);
        run_time = 0;
      }

      // "Run" current process if it exists
      if (current != NULL) {
        current->burst_time--;
        run_time++;
      }
      current_time++;
    }
  }

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n",
         (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n",
         (float)total_response_time / (float)size);

  free(data);
  return 0;
}
