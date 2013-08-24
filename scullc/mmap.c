/*  -*- C -*-
 * mmap.c -- memory mapping for the scullc char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: _mmap.c.in,v 1.13 2004/10/18 18:07:36 corbet Exp $
 */

#include <linux/module.h>

#include <linux/mm.h>		/* everything */
#include <linux/errno.h>	/* error codes */
#include <asm/pgtable.h>

#include "scullc.h"		/* local definitions */


/*
 * open and close: just keep track of how many times the device is
 * mapped, to avoid releasing it.
 */

void scullc_vma_open(struct vm_area_struct *vma)
{
	struct scullc_dev *dev = vma->vm_private_data;

	dev->vmas++;
}

void scullc_vma_close(struct vm_area_struct *vma)
{
	struct scullc_dev *dev = vma->vm_private_data;

	dev->vmas--;
}

/*
 * The nopage method: the core of the file. It retrieves the
 * page required from the scullc device and returns it to the
 * user. The count for the page must be incremented, because
 * it is automatically decremented at page unmap.
 *
 * For this reason, "order" must be zero. Otherwise, only the first
 * page has its count incremented, and the allocating module must
 * release it as a whole block. Therefore, it isn't possible to map
 * pages from a multipage block: when they are unmapped, their count
 * is individually decreased, and would drop to 0.
 */

int scullc_vma_fault(struct vm_area_struct *vma,
                                struct vm_fault *vmf)
{
	unsigned long offset;
	struct scullc_dev *ptr, *dev = vma->vm_private_data;
	struct page *page = NOPAGE_SIGBUS;
	void *pageptr = NULL; /* default to "missing" */

	mutex_lock(&dev->mutex);
	offset = (unsigned long)(vmf->virtual_address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);
	if (offset >= dev->size) goto out; /* out of range */

	/*
	 * Now retrieve the scullc device from the list,then the page.
	 * If the device has holes, the process receives a SIGBUS when
	 * accessing the hole.
	 */
	offset >>= PAGE_SHIFT; /* offset is a number of pages */
	for (ptr = dev; ptr && offset >= dev->qset;) {
		ptr = ptr->next;
		offset -= dev->qset;
	}
	if (ptr && ptr->data) pageptr = ptr->data[offset];
	if (!pageptr) goto out; /* hole or end-of-file */

	/* got it, now increment the count */
	page = virt_to_page(pageptr);
	get_page(page);
  out:
	mutex_unlock(&dev->mutex);
	if (!page)
		return VM_FAULT_SIGBUS;
	vmf->page = page;
	return 0;
}



struct vm_operations_struct scullc_vm_ops = {
	.open =     scullc_vma_open,
	.close =    scullc_vma_close,
	.fault =   scullc_vma_fault,
};


int scullc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct inode *inode = filp->f_dentry->d_inode;

	/* refuse to map if order is not 0 */
	if (scullc_devices[iminor(inode)].order)
		return -ENODEV;

	/* don't do anything here: "nopage" will set up page table entries */
	vma->vm_ops = &scullc_vm_ops;
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_private_data = filp->private_data;
	scullc_vma_open(vma);
	return 0;
}

