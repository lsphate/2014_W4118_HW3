#ifndef _ACCELERATION_H
#define _ACCELERATION_H

#include <linux/unistd.h>
#define NOISE 10
#define WINDOW 20

struct dev_acceleration{
	int x;
	int y;
	int z;
};

struct acc_dlt {
	int dlt_x;
	int dlt_y;
	int dlt_z;
	int strength;
};

struct acc_motion {

     unsigned int dlt_x; /* +/- around X-axis */
     unsigned int dlt_y; /* +/- around Y-axis */
     unsigned int dlt_z; /* +/- around Z-axis */
     
     unsigned int frq;   /* Number of samples that satisfies:
                          sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};

#endif
