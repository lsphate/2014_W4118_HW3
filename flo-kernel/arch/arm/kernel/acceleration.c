#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>

asmlinkage int sys_set_acceleration(struct dev_acceleration __user * acceleration)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

asmlinkage int sys_accevt_create(struct acc_motion __user *acceleration)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

asmlinkage int sys_accevt_signal(struct dev_acceleration __user * acceleration)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}


asmlinkage int sys_accevt_wait(int event_id)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

asmlinkage int sys_accevt_destroy(int event_id)
{
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

      
