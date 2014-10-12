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

int map_init;
spinlock_t CREATE_LOCK, WAIT_LOCK, DESTROY_LOCK;
struct dev_acceleration data;
struct acc_dlt sample, windowCopy[WINDOW];
DEFINE_IDR(accmap);
DEFINE_KFIFO(accFifo, struct dev_acceleration, 2);
DEFINE_KFIFO(dltFifo, struct acc_dlt, roundup_pow_of_two(20));
DECLARE_WAIT_QUEUE_HEAD(acc_wq);

/*
 * Set current device acceleration in kernel.
 * Acceleration is pointer to sensor data in
 * user space. Acceleration value is overwritten
 * every time this is called.
 */

SYSCALL_DEFINE1(set_acceleration,
		struct dev_acceleration __user *, acceleration)
{
	if (copy_from_user(&data,
			acceleration, sizeof(struct dev_acceleration)))
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
	int map_result, map_id;
	struct acc_motion *accevt;
	struct acc_motion_status *newacc;

	accevt = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	if (!accevt)
		return -EFAULT;

	if (copy_from_user(accevt, acceleration, sizeof(struct acc_motion)))
		return -EINVAL;

	newacc = kmalloc(sizeof(struct acc_motion_status), GFP_KERNEL);
	if (!newacc)
		return -EFAULT;

	newacc->condition = 0;
	newacc->user_acc = *accevt;

	if (map_init != 1) {
		idr_init(&accmap);
		map_init = 1;
	}
	map_result = 0;
	map_id = 0;

map_retry:
	if (idr_pre_get(&accmap, GFP_KERNEL) == 0)
		return -ENOMEM;

	spin_lock(&CREATE_LOCK);
	map_result = idr_get_new(&accmap, newacc, &map_id);
	spin_unlock(&CREATE_LOCK);
	if (map_result < 0) {
		if (map_result == -EAGAIN)
			goto map_retry;
		return map_result;
	}
	return map_id;
}

int checkMotion_cb(int id, void *ptr, void *data) {
	struct acc_motion_status *currMotion = ptr;
	struct acc_dlt *windowSamples;
	printk("%d %d %d %d\n", currMotion->user_acc.dlt_x, currMotion->user_acc.dlt_y, currMotion->user_acc.dlt_z, currMotion->user_acc.frq);
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
	int copied, sampleCount;
	int numSamples, frq;
	struct dev_acceleration temp;
	struct dev_acceleration samples[2];

	if (copy_from_user(&data,
			acceleration, sizeof(struct dev_acceleration)))
		return -EINVAL;
	//printk("x: %d, y: %d, z: %d\n", data.x, data.y, data.z);
	if (kfifo_is_full(&accFifo))
		kfifo_out(&accFifo, &temp, 1);

	copied = kfifo_in(&accFifo, &data, 1);
	sampleCount = kfifo_out_peek(&accFifo, &samples, 2);
	if (sampleCount == 2) {
		sample.dlt_x = abs(samples[0].x - samples[1].x);
		sample.dlt_y = abs(samples[0].y - samples[1].y);
		sample.dlt_z = abs(samples[0].z - samples[1].z);
		sample.strength = sample.dlt_x + sample.dlt_y + sample.dlt_z;
		/*printk("dltX %d dltY %d dltZ %d str %d\n",
		sample.dlt_x, sample.dlt_y, sample.dlt_z, sample.strength);
		*/
		/* Check the number of samples.
		 * If already have WINDOW samples
		 * then remove the oldest.
		 */
		struct acc_dlt tempDlt;

		if (kfifo_len(&dltFifo) == WINDOW)
			kfifo_out(&dltFifo, &tempDlt, 1);
		kfifo_in(&dltFifo, &sample, 1);
		numSamples = kfifo_out_peek(&dltFifo, windowCopy, WINDOW);
		frq = 0;
		/*
		 * TODO: put this in a for loop that loops
		 * through the hashmap holding each
		 * acc_motion
		 */
		/*int i;
		
		for (i = 0; i < numSamples; i++) {
			if (windowCopy[i].strength > NOISE)
				frq++;
		}*/
		idr_for_each(&accmap, &checkMotion_cb, windowCopy);
	}
	return 0;
}

SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	/* put process into wait_queue should be done in accevt_create
	 * in here we need to code something that ready to be wake up?
	 */
	int check, *isRunnable;
	struct acc_motion_status *temp;

	check = 0;
	temp = idr_find(&accmap, event_id);
	isRunnable = &temp->condition;

	/*Process should stuck here*/
repeat_waiting:
	do {
		DEFINE_WAIT(__wait);

		for (;;) {
			prepare_to_wait(&acc_wq, &__wait, TASK_INTERRUPTIBLE);
			/*Pervent more than 1 processes access same acc_motion*/
			spin_lock(&WAIT_LOCK);
			if (*isRunnable == 1)
				check = 1;
				break;
			spin_unlock(&WAIT_LOCK);
			schedule();
		}
		finish_wait(&acc_wq, &__wait);
	} while (0);

	if (check != 1)
		goto repeat_waiting;
	else {
		printk("Process wakes up!");
		return 0;
	}
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
	spin_unlock(&DESTROY_LOCK);
	return 0;
}
