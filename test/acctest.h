#ifndef _ACCTEST_H
#define _ACCTEST_H

#include <linux/unistd.h>

struct acc_motion {

     unsigned int dlt_x; /* +/- around X-axis */
     unsigned int dlt_y; /* +/- around Y-axis */
     unsigned int dlt_z; /* +/- around Z-axis */

     unsigned int frq;   /* Number of samples that satisfies:
                          sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};

#endif
