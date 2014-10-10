#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/idr.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
#include <linux/errno.h>

struct dev_acceleration data;
struct idr accmap;
int mapinit = 0;
struct dev_acceleration data;
struct acc_dlt sample;
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
	
	struct acc_motion *accevt;
	accevt = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	if (copy_from_user(&accevt, acceleration, sizeof(struct acc_motion)))
		return -EINVAL;
/*
	struct acc_motion_list newacc;
	newacc.motionlist = *accevt;
	INIT_LIST_HEAD(&newacc.programnode);
        list_add_tail(&(newacc.programnode),&(headnode.programnode));
*/

	if(mapinit==0) {
	idr_init(&accmap);
	mapinit=1;
	}
	int id;
	if(!idr_pre_get(&accmap, GFP_KERNEL))
		return -EAGAIN;
	if(!idr_get_new(&accmap, accevt, &id))
		return -ENOSPC;


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
	struct dev_acceleration samples[2];
	if (copy_from_user(&data, acceleration, sizeof(struct dev_acceleration)))
                return -EINVAL;
	
	printk("x: %d, y: %d, z: %d\n", data.x, data.y, data.z);
	if (kfifo_is_full(&accFifo)) {
		kfifo_out(&accFifo, &temp, 1);
	}
	int copied;
	copied = kfifo_in(&accFifo, &data, 1);
	
	int sampleCount;
	sampleCount = kfifo_out_peek(&accFifo, &samples, 2);
	if ( sampleCount == 2 ) {
		sample.dlt_x = abs(samples[0].x - samples[1].x);
		sample.dlt_y = abs(samples[0].y - samples[1].y);
		sample.dlt_z = abs(samples[0].z - samples[1].z);
		printk("dltX %d dltY %d dltZ %d\n", sample.dlt_x, sample.dlt_y, sample.dlt_z);	
	}
/*
        struct kfifo acc_data_fifo;
	int ret, frqct;
	unsigned int copied;
	
	ret = kfifo_alloc(&acc_data_fifo, 40 * sizeof(struct dev_acceleration), GFP_KERNEL);
	if (ret)		
		return ret;

	if (copy_from_user(&data, acceleration, sizeof(struct dev_acceleration)))
		return -EINVAL; 

	if (kfifo_avail(&acc_data_fifo) < sizeof(struct dev_acceleration))
		return -ENOSPC;

	copied = kfifo_in(&acc_data_fifo, (void *) &data, sizeof(struct dev_acceleration));
	if (copied != sizeof(struct dev_acceleration))
		Maybe display some error here?

	while (kfifo_size(&acc_data_fifo) >= (2 * sizeof(struct dev_acceleration))) {
		struct dev_acceleration prev_data, crnt_data;
		int dx, dy, dz;

		copied = 0;
		copied += kfifo_out(&acc_data_fifo, (void *) &prev_data, sizeof(struct dev_acceleration));
		copied += kfifo_out_peek(&acc_data_fifo, (void *) &crnt_data, sizeof(struct dev_acceleration));
		if (copied != (2 * sizeof(struct dev_acceleration)))
			Maybe display some error here?

		dx = crnt_data.x - prev_data.x;
		dy = crnt_data.y - prev_data.y;
		dz = crnt_data.z - prev_data.z;
	}
*/
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
