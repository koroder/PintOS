#ifndef VM_SWAP_H
#define VM_SWAP_H

#define SEC_PER_PG 8

#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "page.h"
#include "frame.h"

struct block *b;	        // block corresponding to the swap space
struct bitmap *swap_table;	// bitmap used as swap table
struct lock swap_lock;          // swap lock

void swap_init (void);
block_sector_t swap_allocate ( void *kaddr );
void swap_remove ( void *kaddr, block_sector_t sec_no );
bool eviction (void);
struct frame *eviction_clock (void);
struct thread *get_thread_by_tid (tid_t tid);
void swap_free (struct shadow_elem *s);

#endif
