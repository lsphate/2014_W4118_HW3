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
#include <asm/atomic.h>

int map_init, numSamples;
spinlock_t IDR_LOCK, WQ_LOCK;
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
	pr_debug("x: %d, y: %d, z: %d\n", data.x, data.y, data.z);
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
	atomic_set(&(newacc->numProc), 0);
	init_waitqueue_head(&(newacc->eventWQ));

	if (map_init != 1) {
		idr_init(&accmap);
		map_init = 1;
	}
	map_result = 0;
	map_id = 0;

map_retry:
	spin_lock(&IDR_LOCK);
	if (idr_pre_get(&accmap, GFP_KERNEL) == 0)
		return -ENOMEM;
	map_result = idr_get_new(&accmap, newacc, &map_id);
	spin_unlock(&IDR_LOCK);
	if (map_result < 0) {
		if (map_result == -EAGAIN)
			goto map_retry;
		return map_result;
	}
	return map_id;
}

int checkMotion_cb(int id, void *ptr, void *data)
{
	struct acc_motion_status *currMotion = ptr;
	struct acc_dlt *windowSamples = data;
	int i;
	int frq = 0;
	/*printk("%d %d %d %d\n", currMotion->user_acc.dlt_x,
					currMotion->user_acc.dlt_y,
					currMotion->user_acc.dlt_z,
					currMotion->user_acc.frq);*/

	for (i = 0; i < numSamples; i++) {
		if (windowSamples[i].strength > NOISE) {
			if (windowSamples[i].dlt_x < currMotion->user_acc.dlt_x)
				continue;
			if (windowSamples[i].dlt_y < currMotion->user_acc.dlt_y)
				continue;
			if (windowSamples[i].dlt_z < currMotion->user_acc.dlt_z)
				continue;
			frq++;
		}
	}
	if (frq >= currMotion->user_acc.frq) {
		currMotion->condition = 1;
		spin_lock(&WQ_LOCK);
		wake_up(&(currMotion->eventWQ));
		spin_unlock(&WQ_LOCK);
	} else
		currMotion->condition = 0;
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
	struct dev_acceleration temp;
	struct dev_acceleration samples[2];

	if (copy_from_user(&data,
			acceleration, sizeof(struct dev_acceleration)))
		return -EINVAL;
	/*printk("x: %d, y: %d, z: %d\n", data.x, data.y, data.z);*/
	if (kfifo_is_full(&accFifo))
		kfifo_out(&accFifo, &temp, 1);

	copied = kfifo_in(&accFifo, &data, 1);
	if (copied != 1)
		pr_debug("Warning! Short writting detected.\n");

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
		spin_lock(&IDR_LOCK);
		idr_for_each(&accmap, &checkMotion_cb, windowCopy);
		spin_unlock(&IDR_LOCK);
	} else
		pr_debug("Warning! Incorrect reading detected.\n");
	return 0;
}

SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	/* put process into wait_queue should be done in accevt_create
	 * in here we need to code something that ready to be wake up?
	 */
	int *isRunnable;
	struct acc_motion_status *temp;

	temp = idr_find(&accmap, event_id);
	if (!temp)
		return -EINVAL;
	isRunnable = &temp->condition;
	atomic_inc(&(temp->numProc));
	/*Process should stuck here*/
	DEFINE_WAIT(__wait);

	for (;;) {
		prepare_to_wait(&(temp->eventWQ),
				&__wait, TASK_INTERRUPTIBLE);
		spin_lock(&WQ_LOCK);
		if (*isRunnable == 1) {
			spin_unlock(&WQ_LOCK);
			break;
		}
		spin_unlock(&WQ_LOCK);
		if (!signal_pending(current)) {
			schedule();
			continue;
		}
		break;
	}
	finish_wait(&(temp->eventWQ), &__wait);
	atomic_dec(&(temp->numProc));

	pr_debug("Process %d wakes up!\n", event_id);
	return 0;
}

SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
	struct acc_motion_status *status_free;

	spin_lock(&IDR_LOCK);
	status_free = idr_find(&accmap, event_id);
	if(status_free) {
		if(atomic_read(&(status_free->numProc))) {
			status_free->condition = 1;
			wake_up(&(status_free->eventWQ));
		}
		idr_remove(&accmap, event_id);
		kfree(status_free);
		pr_debug("Destroy complete.\n");
	}
	spin_unlock(&IDR_LOCK);

	return 0;
}
