#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct idr accmap;
int mapinit = 0;
spinlock_t WAIT_LOCK;
spinlock_t DESTROY_LOCK;
struct dev_acceleration data;
struct acc_dlt sample;
struct acc_dlt windowCopy[WINDOW];
DEFINE_KFIFO(accFifo, struct dev_acceleration, 2);
DEFINE_KFIFO(dltFifo, struct acc_dlt, roundup_pow_of_two(20));
DECLARE_WAIT_QUEUE_HEAD(acc_wq); 

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

	struct acc_motion_status *newacc;
	newacc = kmalloc(sizeof(struct acc_motion_status), GFP_KERNEL);
	newacc->condition = 0;
	newacc->user_acc.dlt_x = accevt->dlt_x;
	newacc->user_acc.dlt_y = accevt->dlt_y;
	newacc->user_acc.dlt_z = accevt->dlt_z;
	newacc->user_acc.frq = accevt->frq;

	if(mapinit!=1) {
		idr_init(&accmap);
		mapinit=1;
	}

////////spin_lock//////////////////
	spin_lock(&accmap.lock);

	int id;
	if(!idr_pre_get(&accmap, GFP_KERNEL))
		return -EAGAIN;
	if(!idr_get_new(&accmap, newacc, &id))
		return -ENOSPC;

	spin_unlock(&accmap.lock);
///////unlock////////////////////////////
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
		sample.strength = sample.dlt_x + sample.dlt_y + sample.dlt_z;
		printk("dltX %d dltY %d dltZ %d str %d\n", sample.dlt_x, sample.dlt_y, sample.dlt_z, sample.strength);	

		/* Check the number of samples.
		 * If already have WINDOW samples
		 * then remove the oldest.
		 */
		struct acc_dlt tempDlt;
		if (kfifo_len(&dltFifo) == WINDOW) {
			kfifo_out(&dltFifo, &tempDlt, 1);
		}
		kfifo_in(&dltFifo, &sample, 1);
		int numSamples;
		numSamples = kfifo_out_peek(&dltFifo, windowCopy, WINDOW);
		/*
		 * TODO: put this in a for loop that loops
		 * through the hashmap holding each
		 * acc_motion
		 */
		int i;
		int frq = 0;
		for (i=0; i<numSamples; i++) {
			if(windowCopy[i].strength > NOISE) {
				frq++;
			}		
		}
		printk("frq: %d\n", frq);
	}
        return 0;
}



SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	/* put process into wait_queue should be done in accevt_create
	 * in here we need to code something that ready to be wake up?
	 */
	int check, isRunnable;
	struct acc_motion_status *temp;

	check = 0;
	temp = idr_find(&accmap, event_id);
	isRunnable = (int)(&temp->condition); 
	
	/*Process should stuck here*/
	repeat_waiting:
	do {
		DEFINE_WAIT(__wait);
		for (;;) {
			prepare_to_wait(&acc_wq, &__wait, TASK_INTERRUPTIBLE);
			/*Pervent more than 1 processes access same acc_motion*/
			spin_lock(&WAIT_LOCK);
			if (isRunnable)
				check = 1;
				break;
			spin_unlock(&WAIT_LOCK);
			schedule();
		}
		finish_wait(&acc_wq, &__wait);
	} while (0);
	
	if (check == 1) {
		printk("Process wakes up!");
		return 0;
	} else
		goto repeat_waiting;
}

SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
        /*Shuold cleanup: idr, acc_motion, acc_motion_status, anything else?*/
	struct acc_motion_status *status_free;
	struct acc_motion *motion_free;

	spin_lock(&DESTROY_LOCK);

	status_free = idr_find(&accmap, event_id);
	motion_free = &status_free->user_acc;
	idr_remove(&accmap, event_id);
	kfree(motion_free);
	kfree(status_free);
	/*Any error should be handle?*/
	spin_unlock(&DESTROY_LOCK);

	printk("Congrats, your new system call has been called successfully");
        return 0;
}
