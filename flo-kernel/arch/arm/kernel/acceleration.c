#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <asm/uaccess.h>

struct dev_acceleration *data;

SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	struct dev_acceleration data;
	if (copy_from_user(&data, acceleration, sizeof(struct dev_acceleration)))
                return -EINVAL;
	
	printk("Congrats, your new system call has been called successfully");
        return 0;
}

SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

SYSCALL_DEFINE1(accevt_signal, struct dev_acceleration __user *, acceleration)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}


SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}
