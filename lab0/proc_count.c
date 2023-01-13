#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>

static int __init proc_count_init(void)
{
	pr_info("proc_count: init\n");
	return 0;
}

static void __exit proc_count_exit(void)
{
	pr_info("proc_count: exit\n");
}

module_init(proc_count_init);
module_exit(proc_count_exit);

MODULE_AUTHOR("Your name");
MODULE_DESCRIPTION("One sentence description");
MODULE_LICENSE("GPL");
