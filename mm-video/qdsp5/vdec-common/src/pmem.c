/* Copyright (c) 2008 by QUALCOMM, Incorporated.  All Rights Reserved. 
 * QUALCOMM Proprietary and Confidential.
 */
#ifdef QLE_BUILD
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    
    #include <stddef.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/mman.h>
    
    #include "pmem2.h"
    #include "pmem.h"
    #include "../../../../../modules/pmem/inc-user/pmem_module.h"
    
    static int pmem_fd;
    
    int pmem_alloc(struct pmem *pMem, unsigned nSize)
    {
       printf("pmem_alloc : Size - [%d]\n", nSize);
    
       if (!pMem)
          return -1;
    
       struct pmem_region_info sRegion;
       const int nFlags = 5;           // bit 0   disable write through cache
                                       // bit 1   1 MB aligned
                                       // bit 2   attach region
       nSize = (nSize + 4095) & (~4095);
       pMem->fd = (int) pmem2_malloc(nSize,
                                     PMEM_EBI1,
                                     nFlags,
                                     &pMem->data);
       if ((pMem->data == NULL) || ( pMem->fd < 0)) 
       {
          printf("error could not allocate pmem");
          return -1;
       }
       pmem2_get_region_info (pMem->fd, &sRegion);
       pMem->phys = (void*) sRegion.paddr;
       pMem->size = nSize;
        
       printf("PMEM ALLOC: Allocation succesfull\n");
    
       return 0;
    }
    
    void pmem_free(struct pmem *pmem)
    {
        close(pmem->fd);
        pmem->fd = -1;
        munmap(pmem->data, pmem->size);
    }

#else
    #include <stdio.h>
    #include <fcntl.h>
    #include "pmem.h"
    #include "qutility.h"
    
    #ifndef T_WINNT
    	#include <sys/mman.h>
    	#include <sys/ioctl.h>
    	struct file;
    	#include <linux/android_pmem.h>
    #endif
    
    #define USE_SMI 0  /* do NOT enable if you're running the full system */
    
    int pmem_alloc(struct pmem *pmem, unsigned sz)
    {
    #ifdef T_WINNT
    	sz = (sz + 4095) & (~4095);
    	pmem->size=sz;
    	pmem->fd = 0;
    	pmem->data = (void*)malloc(sz);
    	pmem->phys = (unsigned) pmem->data;
    	return 0;
    #else
        struct pmem_region region;
    
    #if USE_SMI
        pmem->fd = open("/dev/pmem_gpu0", O_RDWR);
    #else
        pmem->fd = open("/dev/pmem_adsp", O_RDWR | O_SYNC);
    #endif
        if (pmem->fd < 0) {
            perror("cannot open pmem device");
            return -1;
        }
    
        sz = (sz + 4095) & (~4095);
        pmem->size = sz;
        pmem->data = mmap(NULL, sz, PROT_READ | PROT_WRITE,
                          MAP_SHARED, pmem->fd, 0);
    
        if (pmem->data == MAP_FAILED) {
            perror("pmem mmap failed");
            close(pmem->fd);
            pmem->fd = -1;
            return -1;
        }
    
        if (ioctl(pmem->fd, PMEM_GET_PHYS, &region)) {
            perror("pmem phys lookup failed");
            close(pmem->fd);
            munmap(pmem->data, pmem->size);
            return -1;
        }
        pmem->phys = region.offset;
        printx("PMEM %p (phys=%08x)\n", pmem->data, pmem->phys);
        return 0;
    #endif
    }
    
    void pmem_free(struct pmem *pmem)
    {
    #ifdef T_WINNT
    	pmem->fd = 0;
    	pmem->phys = 0;
    	pmem->size = 0;
    	free(pmem->data);
    	pmem->data = 0;
    #else
        close(pmem->fd);
        pmem->fd = -1;
        munmap(pmem->data, pmem->size);
    #endif
    }
    
#endif //QLE_BUILD



