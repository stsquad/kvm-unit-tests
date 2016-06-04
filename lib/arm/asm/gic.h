/*
 * Copyright (C) 2016, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 */
#ifndef _ASMARM_GIC_H_
#define _ASMARM_GIC_H_

#include <asm/gic-v2.h>
#include <asm/gic-v3.h>

/*
 * gic_init will try to find all known gics, and then
 * initialize the gic data for the one found.
 * returns
 *  0   : no gic was found
 *  > 0 : the gic version of the gic found
 */
extern int gic_init(void);

#endif /* _ASMARM_GIC_H_ */
