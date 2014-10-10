#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/idr.h>

struct dev_acceleration data;
//struct acc_motion_list headnode;
//INIT_LIST_HEAD( &(headnode.programnode) );
//struct acc_motion_list *ptr = &headnode;

struct idr accmap;
int mapinit = 0;
//idr_init (&accmap);


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
