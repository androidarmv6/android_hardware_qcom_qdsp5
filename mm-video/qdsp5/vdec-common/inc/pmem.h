/* Copyright (c) 2008 by QUALCOMM, Incorporated.  All Rights Reserved. 
 * QUALCOMM Proprietary and Confidential.
 */
#ifndef _PMEM_SIMPLE_H_
#define _PMEM_SIMPLE_H_

#define pmem_alloc vdec_pmem_alloc
#define pmem_free vdec_pmem_free

struct pmem
{
    void *data;
    int fd;

    unsigned phys;
    unsigned size;
};

int pmem_alloc(struct pmem *out, unsigned sz);
void pmem_free(struct pmem *pmem);

#endif
    
