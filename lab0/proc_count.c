#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>

static struct proc_dir_entry *proc_entry;

static int proc_count_show(struct seq_file *m, void *v) {
  int proc_count = 0;

  struct task_struct *task;

  for_each_process(task) { proc_count++; }

  seq_printf(m, "%d\n", proc_count);
  return 0;
}

static int __init proc_count_init(void) {
  pr_info("proc_count: init\n");
  proc_entry = proc_create_single("count", 0, NULL, proc_count_show);

  if (!proc_entry) {
    return -ENOMEM;
  }

  return 0;
}

static void __exit proc_count_exit(void) {
  pr_info("proc_count: exit\n");
  proc_remove(proc_entry);
}

module_init(proc_count_init);
module_exit(proc_count_exit);

MODULE_AUTHOR("Paul Zhang");
MODULE_DESCRIPTION("/proc/count returns the number of running processes");
MODULE_LICENSE("GPL");
