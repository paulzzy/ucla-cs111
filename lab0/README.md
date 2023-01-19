# proc_count: A Kernel Seedling

proc_count is a kernel module that creates /proc/count, which returns the
number of running processes.

Before following any steps below, make sure the working directory contains the
source code (e.g. Makefile).

## Building

Run `make` to build the kernel module. One of the output files should be
proc_count.ko.

## Running

Insert the kernel module with `sudo insmod proc_count.ko`. Use `cat /proc/count`
to display the number of running processes.

## Cleaning Up

Remove the kernel module with `sudo rmmod proc_count`. Clean up build artifacts
by running `make clean`.

## Testing

proc_count was tested on kernel version 5.14.8-arch1-1.

Use `python -m unittest` to test proc_count. Or ensure the outputs of `cat
/proc/count` and `ps aux | wc -l` are the same.

Use `sudo dmesg | grep proc_count` to check for successful insertion or removal
of proc_count.

Use `modinfo proc_count.ko` to check information about the built kernel module.
E.g., the "vermagic" line details which kernel version the module was built for.
