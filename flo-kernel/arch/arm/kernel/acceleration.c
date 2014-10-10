#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <asm/uaccess.h>
#include <linux/kfifo.h>
#include <linux/log2.h>

struct dev_acceleration data;
DEFINE_KFIFO(accFifo, struct dev_acceleration, 2);
DEFINE_KFIFO(dltFifo, struct acc_dlt, roundup_pow_of_two(20));
/*
 * Set current device acceleration in kernel.
 * Acceleration is pointer to sensor data in
 * user space. Acceleration value is overwritten
 * every time this is called.
 */
SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	if (copy_from_user(&data, acceleration, sizeof(struct dev_acceleration)))
                return -EINVAL;
	printk("x: %d, y: %d, z: %d\n", data.x, data.y, data.z);
	
		
        return 0;
}

/* Create an event based on motion
 * If frq exceeds WINDOW, cap frq at WINDOW.
 * Returns event_id on success and error on failure.
 */

SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{
	/* TODO:
	 * kmalloc a new struct acc_motion
	 * copy_from_user the passed acc_motion
	 * add it to a larger data structure (hash table?)
	 */

	
        printk("Congrats, your new system call has been called successfully");
        return 0;
}

/* take sensor data from user and store in kernel
 * calculate the motion event
 * notify all events with matching event
 * unlblock all processes waiting
 */
SYSCALL_DEFINE1(accevt_signal, struct dev_acceleration __user *, acceleration)
{
	/* TODO:
	 * Take in dev_accleration and copy_from_user
	 * Subtract from previous dev_acceleration stored
	 * in buffer to get difference in acc. If exceeds noise
	 * add to buffer of size WINDOW. Check events using updated
	 * buffer. Notify events with frq exceeded. Unblock those
	 * that match.
	 */
	struct dev_acceleration temp;
	
	if (copy_from_user(&data, acceleration, sizeof(struct dev_acceleration)))
                return -EINVAL;
	printk("kfifo size: %d\n", kfifo_len(&accFifo));
	printk("adding to kfifo\n");
	if (kfifo_is_full(&accFifo)) {
		printk("kfifo was full so popping\n");
		kfifo_out(&accFifo, &temp, sizeof(struct dev_acceleration));
	}
	kfifo_in(&accFifo, &data, sizeof(struct dev_acceleration));
	kfifo_peek(&accFifo, &temp);
	printk("x: %d, y: %d, z: %d\n", temp.x, temp.y, temp.z);
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
